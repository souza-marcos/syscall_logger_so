#include <linux/init.h> /* For the macros */
#include <linux/module.h>
#include <linux/printk.h>

static int my_data __initdata = 3;

static int __init hello2main(void){
	pr_info("[Hello World] Hi mom! %d\n", my_data);
	return 0;
}

static void __exit hello2exit(void){
	pr_info("[Hello World] That's all folks!\n");
}

module_init(hello2main);
module_exit(hello2exit);
MODULE_LICENSE("GLP");
MODULE_AUTHOR("Marcos");
MODULE_DESCRIPTION("Testing the loading of kernel modules");

// Cool Commands
// modinfo hello-1.ko
// journalctl --since "1 hour ago" | grep kernel