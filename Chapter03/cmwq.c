#define pr_fmt(fmt) "PACKT-03-cmwq: " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>    /* for work queue */
#include <linux/slab.h>         /* for kmalloc() */

static struct workqueue_struct *wq;
 
struct work_data {
    struct work_struct my_work;
    int the_data;
};
 
static void work_handler(struct work_struct *work)
{
    struct work_data * my_data = container_of(work, struct work_data, my_work);
    pr_info("CMWQ module handler: %s, data is %d\n",
         __func__, my_data->the_data);
    kfree(my_data);
}

static int __init my_init(void)
{
    struct work_data * my_data;

    pr_info("CMWQ  module init: %s %d\n", __func__, __LINE__);
    wq = alloc_workqueue("cmwp-example",
                         /* we want it to be unbound and high priority */
                         WQ_UNBOUND | WQ_HIGHPRI,
                         0);
    my_data = kmalloc(sizeof(struct work_data), GFP_KERNEL);

    my_data->the_data = 34;

    INIT_WORK(&my_data->my_work, work_handler);
    queue_work(wq, &my_data->my_work);
 
    return 0;
}

static void __exit my_exit(void)
{
    flush_workqueue(wq);
    destroy_workqueue(wq);
    pr_info("Work queue module exit: %s %d\n", __func__, __LINE__);
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_DESCRIPTION("Dedicated workqueue example");
