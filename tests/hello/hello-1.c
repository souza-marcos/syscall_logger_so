#include <linux/module.h>
#include <linux/printk.h>

int init_module(void){
	pr_info("[Hello World] Hi mom!\n");
	return 0;
}

void cleanup_module(void){
	pr_info("[Hello World] That's all folks!\n");
}
	

MODULE_LICENSE("GLP");

// Cool Commands
// modinfo hello-1.ko
// journalctl --since "1 hour ago" | grep kernel