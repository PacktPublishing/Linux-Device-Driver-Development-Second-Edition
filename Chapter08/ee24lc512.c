#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <asm/uaccess.h>


/* 
 * The structure to represent 'eep_dev' devices.
 *  data - data buffer;
 *  block_size - maximum number of bytes that can be read or written 
 *    in one call;
 *  eep_mutex - a mutex to protect this device;
 *  users: the number of time this device is being opened
 */
struct eep_dev {
    unsigned char *data;
    struct i2c_client *client;
    struct mutex eep_mutex;
    struct list_head    device_entry;
    dev_t       devt;
    unsigned    users;
};

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

#define EEP_DEVICE_NAME     "packt-mem"
#define EEP_PAGE_SIZE           128
#define EEP_SIZE            1024*64 /* 24LC512 is 64KB sized */

static struct class *eep_class = NULL;
static int probed_ndevices = 0;

int  eep_open(struct inode *inode, struct file *filp)
{
    struct eep_dev *eeprom = NULL;
    int         err = -ENXIO;

    mutex_lock(&device_list_lock);
    list_for_each_entry(eeprom, &device_list, device_entry) {
        if (eeprom->devt == inode->i_rdev) {
            err = 0;
            break;
        }
    }

    if (err) {
        pr_warn("eeprom: nothing for major %d\n", imajor(inode));
        goto err_find_dev;
    }

    /* if opened the 1st time, allocate the buffer */
    if (eeprom->data == NULL) {
        eeprom->data = kzalloc(EEP_SIZE, GFP_KERNEL);
        if (!eeprom->data) {
            pr_err("[target] open(): out of memory\n");
            err = -ENOMEM;
            goto err_alloc_data_buf;
        }
    }

    eeprom->users++;
    /* store a pointer to struct eep_dev here for other methods */
    filp->private_data = eeprom;
    mutex_unlock(&device_list_lock);
    return 0;

err_alloc_data_buf:
    kfree(eeprom->data);
    eeprom->data = NULL;
err_find_dev:
    mutex_unlock(&device_list_lock);
    return err;
}

/*
 * Release is called when device node is closed
 */
int eep_release(struct inode *inode, struct file *filp)
{
    struct eep_dev *eeprom;

    mutex_lock(&device_list_lock);
    eeprom = filp->private_data;

    /* last close? */
    eeprom->users--;
    if (!eeprom->users) {
        kfree(eeprom->data);
        eeprom->data = NULL ; /* Never forget to set to 'NULL' after freeing */
    }

    mutex_unlock(&device_list_lock);
    return 0;
}

ssize_t  eep_read(struct file *filp, char __user *buf,
                    size_t count, loff_t *f_pos)
{
    struct eep_dev *eeprom = filp->private_data;
    struct i2c_msg msg[2];
    ssize_t retval = 0;
    int _reg_addr;
    u8 reg_addr[2];

    if (mutex_lock_killable(&eeprom->eep_mutex))
        return -EINTR;

    if (*f_pos >= EEP_SIZE) /* EOF */
        goto end_read;

    if (*f_pos + count > EEP_SIZE)
        count = EEP_SIZE - *f_pos;

    if (count > EEP_SIZE)
        count = EEP_SIZE;

    _reg_addr = (int)(*f_pos);
    reg_addr[0] = (u8)(_reg_addr >> 8);
    reg_addr[1] = (u8)(_reg_addr & 0xFF);

    msg[0].addr = eeprom->client->addr;
    msg[0].flags = 0;                    /* Write */
    msg[0].len = 2;                      /* Address is 2byte coded */
    msg[0].buf = reg_addr;          

    msg[1].addr = eeprom->client->addr;
    msg[1].flags = I2C_M_RD;             /* We need to read */
    msg[1].len = count; 
    msg[1].buf = eeprom->data;

    if (i2c_transfer(eeprom->client->adapter, msg, 2) < 0)
        pr_err("ee24lc512: i2c_transfer failed\n"); 
 
    if (copy_to_user(buf, eeprom->data, count) != 0) {
        retval = -EIO;
        goto end_read;
    }

    retval = count;
    *f_pos += count ;

end_read:
    mutex_unlock(&eeprom->eep_mutex);
    return retval;
}

int transacWrite(struct eep_dev *eeprom,
        int _reg_addr, unsigned char *data,
        int offset, unsigned int len)
{
    unsigned char tmp[len + 2];
    struct i2c_msg msg[1];

    tmp[0] =  (u8)(_reg_addr >> 8);
    tmp[1] =  (u8)(_reg_addr & 0xFF);
    memcpy (tmp + 2, &(data[offset]), len);

    msg[0].addr = eeprom->client->addr;
    msg[0].flags = 0;                    /* Write */
    msg[0].len = len + 2; /* Address is 2 bytes coded */
    msg[0].buf = tmp;

    if (i2c_transfer(eeprom->client->adapter, msg, 1) < 0){
        pr_err("ee24lc512: i2c_transfer failed\n");  
        return -1;
     }
     return len;
}

ssize_t  eep_write(struct file *filp, const char __user *buf,
                    size_t count,  loff_t *f_pos)
{
    int i, _reg_addr, offset, remain_in_page, nb_page, last_remain;
    struct eep_dev *eeprom = filp->private_data;
    ssize_t retval = 0;

    if (mutex_lock_killable(&eeprom->eep_mutex))
        return -EINTR;

    if (*f_pos >= EEP_SIZE) {
        retval = -EINVAL;
        goto end_write;
    }

    if (*f_pos >= EEP_SIZE) {
        /* Writing beyond the end of the buffer is not allowed. */
        retval = -EINVAL;
        goto end_write;
    }

    if (*f_pos + count > EEP_SIZE)
        count = EEP_SIZE - *f_pos;

    if (count > EEP_SIZE)
        count = EEP_SIZE;

    if (copy_from_user(eeprom->data, buf, count) != 0) {
        retval = -EFAULT;
        goto end_write;
    }

    _reg_addr =  (int)(*f_pos);
    offset = 0;
    remain_in_page = (EEP_PAGE_SIZE - (_reg_addr % EEP_PAGE_SIZE)) % EEP_PAGE_SIZE;
    nb_page = (count - remain_in_page) / EEP_PAGE_SIZE;
    last_remain = (count - remain_in_page) % EEP_PAGE_SIZE ;

    if (remain_in_page > 0){
        retval = transacWrite(eeprom, _reg_addr, eeprom->data, offset, remain_in_page);
        if (retval < 0)
            goto end_write;
        offset += remain_in_page;
        _reg_addr += remain_in_page;
        retval = offset;
        mdelay(10);
    }

    if (nb_page) {
        for (i=0; i < nb_page; i++){
            retval = transacWrite(eeprom, _reg_addr, eeprom->data, offset, EEP_PAGE_SIZE);
            if (retval < 0)
                goto end_write;
            offset += EEP_PAGE_SIZE;
            _reg_addr += EEP_PAGE_SIZE;
            retval = offset;
            mdelay(10);
        }
    }

    if (last_remain > 0){
        retval = transacWrite(eeprom, _reg_addr, eeprom->data, offset, last_remain);
        if (retval < 0)
            goto end_write;
        offset += last_remain;
        _reg_addr += last_remain;
        retval = offset;
        mdelay(10);
    }

    *f_pos += count;

end_write:
    mutex_unlock(&eeprom->eep_mutex);
    return retval;
}

loff_t eep_llseek(struct file *filp, loff_t off, int whence)
{
    loff_t newpos = 0;

    switch (whence) {
      case 0: /* SEEK_SET */
        newpos = off;
        break;

      case 1: /* SEEK_CUR */
        newpos = filp->f_pos + off;
        break;

      case 2: /* SEEK_END - Not supported */
        return -EINVAL;

      default: /* can't happen */
        return -EINVAL;
    }
    if (newpos < 0 || newpos > EEP_SIZE)
        return -EINVAL;

    filp->f_pos = newpos;
    return newpos;
}

struct file_operations eep_fops = {
	.owner =    THIS_MODULE,
	.read =     eep_read,
	.write =    eep_write,
	.open =     eep_open,
	.release =  eep_release,
	.llseek =   eep_llseek,
};

#ifdef CONFIG_OF
static const struct of_device_id eeprom_dt_ids[] = {
    { .compatible = "microchip,ee24lc512" },
    {},
};
MODULE_DEVICE_TABLE(of, eeprom_dt_ids);
#endif

static int ee24lc512_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
    int major;
    unsigned char data[5];
    u8 reg_addr[2];
    struct i2c_msg msg[2];

    int err = 0;
    struct eep_dev *eeprom = NULL;
    struct device *device = NULL;

    if (!i2c_check_functionality(client->adapter,
            I2C_FUNC_SMBUS_BYTE_DATA))
        return -EIO;

   /* 
    * We send a simple i2c transaction. If it fails,
    * it means there is no eeprom
    */
    reg_addr[0] =  0x00;
    reg_addr[1] =  0x00;

    msg[0].addr = client->addr;
    msg[0].flags = 0;                    /* Write */
    msg[0].len = 2;                      /* Address is 2byte coded */
    msg[0].buf = reg_addr;          

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;             /* We need to read */
    msg[1].len = 5; //count; 
    msg[1].buf = data;

    if (i2c_transfer(client->adapter, msg, 2) < 0) {
        pr_err("ee24lc512: i2c_transfer failed\n");
        return -ENODEV;
    }

    eeprom = devm_kzalloc(&client->dev, sizeof(*eeprom), GFP_KERNEL);
    if (!eeprom)
        return -ENOMEM;

    major = register_chrdev(0, "eeprom", &eep_fops);
    if (major < 0) {
        pr_err("[target] register_chrdev() failed\n");
        return major;
    }

    /* Construct devices */
    eeprom->data = NULL;
    eeprom->client = client;
    mutex_init(&eeprom->eep_mutex);
    INIT_LIST_HEAD(&eeprom->device_entry);

    eeprom->devt = MKDEV(major, 0);
    device = device_create(eep_class, NULL, /* no parent device */
        eeprom->devt, NULL, /* no additional data */
        EEP_DEVICE_NAME "_%d", probed_ndevices++);

    if (IS_ERR(device)) {
        err = PTR_ERR(device);
        pr_err("[target] Error %d while trying to create %s%d",
                err, EEP_DEVICE_NAME, --probed_ndevices);
        goto fail;
    }

    mutex_lock(&device_list_lock);
    list_add(&eeprom->device_entry, &device_list);
    i2c_set_clientdata(client, eeprom);
    mutex_unlock(&device_list_lock);

    return 0; /* success */

fail:
    if (eeprom)
        kfree(eeprom);
    return err;
}

static int ee24lc512_remove(struct i2c_client *client)
{
    struct eep_dev *eeprom = i2c_get_clientdata(client);

    /* prevent new opens */
    mutex_lock(&device_list_lock);
    list_del(&eeprom->device_entry);
    device_destroy(eep_class, eeprom->devt);
    if (eeprom->users == 0)
        kfree(eeprom);
    mutex_unlock(&device_list_lock);

    return 0;
}

static const struct i2c_device_id ee24lc512_id[] = {
	{"ee24lc512", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, ee24lc512_id);

static struct i2c_driver ee24lc512_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ee24lc512",
		.of_match_table = of_match_ptr(eeprom_dt_ids),
	},
	.probe = ee24lc512_probe,
	.remove = ee24lc512_remove,
	.id_table = ee24lc512_id,
};

static int __init eeprom_drv_init(void)
{
    int status;

    /* Claim our 256 reserved device numbers.  Then register a class
     * that will key udev/mdev to add/remove /dev nodes.  Last, register
     * the driver which manages those device numbers.
     */
    eep_class = class_create(THIS_MODULE, "eeprom");
    if (IS_ERR(eep_class))
        return PTR_ERR(eep_class);

    status = i2c_register_driver(THIS_MODULE, &ee24lc512_i2c_driver);
    if (status < 0)
        class_destroy(eep_class);

    return status;
}
module_init(eeprom_drv_init);

static void __exit eeprom_drv_exit(void)
{
    i2c_del_driver(&ee24lc512_i2c_driver);
    class_destroy(eep_class);
}
module_exit(eeprom_drv_exit);

MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_LICENSE("GPL");
