#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h> /* For the macros */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h> /* The list of system calls */
#include <linux/cred.h>   /* current_uid() */
#include <linux/uidgid.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

/**
 * Access the sys_call_table varies by the kernel version
 *  - Up to v5.4   : Manual Symbol lookup
 *  - v5.5 to v5.6 : Use kallsyms_lookup_name()
 *	- v5.7 +       : Kprobes or specific kernel module parameter
 */

#include <linux/kprobes.h>

static uid_t uid = -1;
module_param(uid, int, 0644);

static char *syscall_sym = "__x64_sys_openat"; // Look in /proc/kallsyms
module_param(syscall_sym, charp, 0644);

static int sys_call_kprobe_pre_handler(struct kprobe *p, struct pt_regs *regs){
	if(__kuid_val(current_uid()) != uid){
		return 0;
	}
	pr_info("%s called by %d\n", syscall_sym, uid);
	return 0;
}

static struct kprobe syscall_kprobe = {
	.symbol_name = "__x64_sys_openat",
	.pre_handler = sys_call_kprobe_pre_handler,
};

static int __init logsys_start(void){
	int err = 0;
	syscall_kprobe.symbol_name = syscall_sym;
	err = register_kprobe(&syscall_kprobe);
	if(err){
		pr_err("register_kprobe() on %s failed: %d\n", syscall_sym, err);
		pr_err("Check the symbol name from 'syscall_sym' parameter.\n");
		return err;
	}
	pr_info("Spying on UID:%d\n", uid);
	return 0;
}

static void __exit syscall_steal_end(void){
	unregister_kprobe(&syscall_kprobe);
}

module_init(logsys_start);
module_exit(syscall_steal_end);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marcos");
MODULE_DESCRIPTION("Testing the loading of kernel modules");

// To use this module
// sudo insmod syscall-steal.ko uid=1000 