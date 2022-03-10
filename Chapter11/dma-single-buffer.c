#define pr_fmt(fmt)	"DMA-TEST: " fmt

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/dmaengine.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/delay.h>

static u32 *wbuf;
static u32 *rbuf;
static int dma_result;
static int gMajor; /* major number of device */

static struct class *dma_test_class;
static struct completion dma_m2m_ok;
static struct dma_chan *dma_m2m_chan;

static void dev_release(struct device *dev)
{
	pr_info( "releasing dma capable device\n");
}

// static u64 _dma_mask = DMA_BIT_MASK(64);
static struct device dev = {
	.release = dev_release,
	.coherent_dma_mask = ~0, 	/* dma_alloc_coherent(): allow any address */
	.dma_mask = &dev.coherent_dma_mask,	/* other APIs: use the same mask as coherent */
};

/* could have been st with dma_set_mask_and_coherent() */

/* we need page aligned buffers */
#define DMA_BUF_SIZE  2 * PAGE_SIZE

int dma_open(struct inode * inode, struct file * filp)
{
	init_completion(&dma_m2m_ok);

	wbuf = kzalloc(DMA_BUF_SIZE, GFP_KERNEL | GFP_DMA);
	if(!wbuf) {
		pr_err("Failed to allocate wbuf!\n");
		return -ENOMEM;
	}

	rbuf = kzalloc(DMA_BUF_SIZE, GFP_KERNEL | GFP_DMA);
	if(!rbuf) {
		kfree(wbuf);
		pr_err("Failed to allocate rbuf!\n");
		return -ENOMEM;
	}

	return 0;
}

int dma_release(struct inode * inode, struct file * filp)
{
	kfree(wbuf);
	kfree(rbuf);
	return 0;
}

/* remove the #if ... #endif to have data dumped in the dmesg buffer */
__maybe_unused static void data_dump(const char *prefix, u8 *d, size_t count)
{
#if 0
	size_t i;

	pr_info("Dumping %s\n", prefix);
	for (i = 0; i < count; i += 8)
		pr_info(
			"[%04d] %02x %02x %02x %02x %02x %02x %02x %02x\n",
			i, d[i], d[i + 1], d[i + 2], d[i + 3],
			d[i + 4], d[i + 5], d[i + 6], d[i + 7]);
#endif
}

ssize_t dma_read (struct file *filp, char __user * buf, size_t count,
		 loff_t * offset)
{
	pr_info("DMA result: %d!\n", dma_result);
	return 0;
}

static void dma_m2m_callback(void *data)
{
	pr_info("in %s\n",__func__);

	complete(&dma_m2m_ok);
}

ssize_t dma_write(struct file * filp, const char __user * buf, size_t count,
                                loff_t * offset)
{
	u32 *index, i;
	size_t err = count;
	dma_cookie_t cookie;
	dma_cap_mask_t dma_m2m_mask;
	dma_addr_t dma_src, dma_dst;
	struct dma_slave_config dma_m2m_config = {0};
	struct dma_async_tx_descriptor *dma_m2m_desc;

	pr_info("Initializing buffer\n");
	index = wbuf;
	for (i = 0; i < DMA_BUF_SIZE/4; i++) {
		*(index + i) = 0x56565656;
	}
	data_dump("WBUF initialized buffer", (u8*)wbuf, DMA_BUF_SIZE);
	pr_info("Buffer initialized\n");

	/* 1- Initialize capabilities and request a DMA channel */
	dma_cap_zero(dma_m2m_mask);
	dma_cap_set(DMA_MEMCPY, dma_m2m_mask);
	/* dma_m2m_chan = dma_request_channel(dma_m2m_mask, NULL, NULL); */
	dma_m2m_chan = dma_request_chan_by_mask(&dma_m2m_mask);
	if (!dma_m2m_chan) {
		pr_err("Error requesting the DMA memory to memory channel\n");
		return -EINVAL;
	} else {
		pr_info("Got DMA channel %d\n", dma_m2m_chan->chan_id);
	}

	/* 2- Set slave and controller specific parameters */
	dma_m2m_config.direction = DMA_MEM_TO_MEM;
	dma_m2m_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dmaengine_slave_config(dma_m2m_chan, &dma_m2m_config);
	pr_info("DMA channel configured\n");

	dma_src = dma_map_single(&dev, wbuf, DMA_BUF_SIZE, DMA_TO_DEVICE);
	if (dma_mapping_error(&dev, dma_src)) {
		pr_err("Could not map src buffer\n");
		err = -ENOMEM;
		goto channel_release;
	}
	dma_dst = dma_map_single(&dev, rbuf, DMA_BUF_SIZE, DMA_FROM_DEVICE);
	if (dma_mapping_error(&dev, dma_dst)) {
		pr_err("Could not map dst buffer\n");
		dma_unmap_single(&dev, dma_src, DMA_BUF_SIZE, DMA_TO_DEVICE);
		err = -ENOMEM;
		goto channel_release;
	}
	pr_info("DMA mappings created\n");

	/* 3- Get a descriptor for the transaction. */
	dma_m2m_desc =
		dma_m2m_chan->device->device_prep_dma_memcpy(
			dma_m2m_chan, dma_dst, dma_src, DMA_BUF_SIZE, 0);
	if (!dma_m2m_desc) {
		pr_err("error in prep_dma_sg\n");
		err = -EINVAL;
		goto dma_unmap;
	}
	dma_m2m_desc->callback = dma_m2m_callback;

	/* 4- Submit the transaction */
	cookie = dmaengine_submit(dma_m2m_desc);
	if (dma_submit_error(cookie)) {
		pr_err("Unable to submit the DMA coockie\n");
		err = -EINVAL;
		goto dma_unmap;
	}
	pr_info("Got this cookie: %d\n", cookie);
 
	/* 5- Issue pending DMA requests and wait for callback notification */
	dma_async_issue_pending(dma_m2m_chan);
	pr_info("waiting for DMA transaction...\n");

	/* One can use wait_for_completion_timeout() also */
	wait_for_completion(&dma_m2m_ok);

dma_unmap:
	/* we do not care about the source anymore */
	dma_unmap_single(&dev, dma_src, DMA_BUF_SIZE, DMA_TO_DEVICE);
	/* unmap the DMA memory destination for CPU access. This will sync the buffer */
	dma_unmap_single(&dev, dma_dst, DMA_BUF_SIZE, DMA_FROM_DEVICE);

	/* 
	 * if no error occured, then we are safe to access the buffer.
	 * remember, the buffer must be synced first, and dma_unmap_single() did it.
	 */
	if (err >= 0) {
		pr_info("Checking if DMA succeed ...\n");

		for (i = 0; i < DMA_BUF_SIZE/4; i++) {
			if (*(rbuf+i) != *(wbuf+i)) {
				pr_err("Single DMA buffer copy falled!,r=%x,w=%x,%d\n",
					*(rbuf+i), *(wbuf+i), i);
				return err;
			}
		}

		pr_info("buffer copy passed!\n");
		dma_result = 1;
		data_dump("RBUF DMA buffer", (u8*)rbuf, DMA_BUF_SIZE);
    	}

channel_release:
	dma_release_channel(dma_m2m_chan);
	dma_m2m_chan = NULL;

	return err;
}

struct file_operations dma_fops = {
	.open = dma_open,
	.read = dma_read,
	.write = dma_write,
	.release = dma_release,
};

int __init dma_init_module(void)
{
	int error;
	struct device *dma_test_dev;

	/* register a character device */
	error = register_chrdev(0, "dma_test", &dma_fops);
	if (error < 0) {
		pr_err("DMA test driver can't get major number\n");
		return error;
	}
	gMajor = error;
	pr_info("DMA test major number = %d\n",gMajor);

	dma_test_class = class_create(THIS_MODULE, "dma_test");
	if (IS_ERR(dma_test_class)) {
		pr_err("Error creating dma test module class.\n");
		unregister_chrdev(gMajor, "dma_test");
		return PTR_ERR(dma_test_class);
	}

	dma_test_dev = device_create(dma_test_class, NULL,
				     MKDEV(gMajor, 0), NULL, "dma_test");

	if (IS_ERR(dma_test_dev)) {
		pr_err("Error creating dma test class device.\n");
		class_destroy(dma_test_class);
		unregister_chrdev(gMajor, "dma_test");
		return -1;
	}

	dev_set_name(&dev, "dma-test-dev");
	device_register(&dev);

	pr_info("DMA test Driver Module loaded\n");
	return 0;
}

static void dma_cleanup_module(void)
{
	unregister_chrdev(gMajor, "dma_test");
	device_destroy(dma_test_class, MKDEV(gMajor, 0));
	class_destroy(dma_test_class);
	device_unregister(&dev);

	pr_info("DMA test Driver Module Unloaded\n");
}

module_init(dma_init_module);
module_exit(dma_cleanup_module);

MODULE_AUTHOR("John Madieu <john.madieu@laabcsmart.com>");
MODULE_DESCRIPTION("DMA test driver");
MODULE_LICENSE("GPL");
