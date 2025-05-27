#include <linux/fs.h>
#include <linux/init.h> /* For the macros */
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/printk.h>

static struct kobject *mymodule;

static int counter = 0;

static ssize_t counter_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
	return sprintf(buf, "%d\n", counter);
}

static ssize_t counter_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t c){
	sscanf(buf, "%d", &counter);
	return c;
}

static struct kobj_attribute counter_attribute = __ATTR(counter, 0660, counter_show, counter_store);

static int __init hellosysfs_init(void){
	int error = 0;
	pr_info("[hellosysfs] Initialized\n");

	mymodule = kobject_create_and_add("hellosysfs", kernel_kobj);
	if(!mymodule) 
		return -ENOMEM;

	error = sysfs_create_file(mymodule, &counter_attribute.attr);
	if(error){
		kobject_put(mymodule);
		pr_info("failed to create the counter file in /sys/kernel/hellosysfs\n");
	}

	return error;
}

static void __exit hellosysfs_exit(void){
	pr_info("[hellosysfs] Exit success, counter: %d\n", counter);
	kobject_put(mymodule);
}

module_init(hellosysfs_init);
module_exit(hellosysfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marcos");
MODULE_DESCRIPTION("Testing the loading of kernel modules");

// Cool Commands
// modinfo hello-1.ko
// journalctl --since "1 hour ago" | grep kernel