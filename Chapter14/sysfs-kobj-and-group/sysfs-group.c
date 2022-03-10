#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/kobject.h>

static int foo = 0;
static int bar = 0;

static ssize_t demo_show(struct kobject *kobj, struct kobj_attribute *attr,
                         char *buf)
{
    size_t len = 0;
    int val = 0;

    len = scnprintf(buf, PAGE_SIZE,
             "%s:\t", attr->attr.name);

    if (strcmp(attr->attr.name, "foo") == 0)
        val = foo;
    else 
        val = bar;

    len += scnprintf(buf + len, PAGE_SIZE - len, "%d\n", val);
  
    return len;
}

static ssize_t demo_store(struct kobject *kobj, struct kobj_attribute *attr,
                          const char *buf, size_t len)
{
    int val;

    if (!len || !sscanf(buf, "%d", &val))
        return -EINVAL;

    if (strcmp(attr->attr.name, "foo") == 0)
       foo = val;
    else 
        bar = val;

    return len;
}

static struct kobj_attribute foo_attr =
    __ATTR(foo, 0660, demo_show, demo_store);
static struct kobj_attribute bar_attr =
    __ATTR(bar, 0660, demo_show, demo_store);

/* attrs is an array of pointers to attributes */
static struct attribute *demo_attrs[] = {
    &foo_attr.attr,
    &bar_attr.attr,
    NULL,};

static struct attribute_group my_attr_group = {
    .attrs = demo_attrs,
    /*.bin_attrs = demo_bin_attrs,*/
};

static struct kobject *demo_kobj;

static int __init hello_module_init(void)
{
    int err = -1;
    pr_info("sysfs group and kobject test: init\n");

    /*
     * Create a simple kobject with the name of "packt",
     * located under /sys/kernel/
     */
    demo_kobj = kobject_create_and_add("packt", kernel_kobj);
    if (!demo_kobj)
        return -ENOMEM;

    err = sysfs_create_group(demo_kobj, &my_attr_group);
    if (err)
        kobject_put(demo_kobj);

    return err;
}

static void __exit hello_module_exit(void)
{
    if (demo_kobj)
        kobject_put(demo_kobj);

    pr_info("sysfs group and kobject test: exit\n");
}

module_init(hello_module_init);
module_exit(hello_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
