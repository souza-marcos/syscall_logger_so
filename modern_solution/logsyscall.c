#include <linux/kernel.h>
#include <linux/init.h> /* For the macros */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h> /* The list of system calls */
#include <linux/cred.h>   /* current_uid() */
#include <linux/uidgid.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/kprobes.h>
#include <linux/string.h>

static uid_t uid = -1;
module_param(uid, int, 0644);
static char *syscall_syms = NULL; // Look in /proc/kallsyms
module_param(syscall_syms, charp, 0644);
static struct kprobe *kprobes = NULL;
static char **syscall_names = NULL;
static int num_syscalls = 0;

static int sys_call_kprobe_pre_handler(struct kprobe *p, struct pt_regs *regs){
	if(__kuid_val(current_uid()) != uid){
		return 0;
	}
    const char *syscall_name = (const char *) p->symbol_name;

    pr_info("%s called by %d\n", syscall_name, uid);
	return 0;
}

static int __init logsys_start(void){
	int err = 0, i = 0;
    char *str, *token, *saveptr = NULL;

    if(!syscall_syms){
        pr_err("syscall_syms parameter is required\n");
        return -EINVAL;
    }

    str = kstrdup(syscall_syms, GFP_KERNEL);
    if(!str) 
        return -ENOMEM;

    num_syscalls = 0;
    saveptr = str;
    while ((token = strsep(&saveptr, ",")) != NULL) {
        if (*token != '\0') 
            num_syscalls++;
    }
    kfree(str);
    
    if(num_syscalls == 0){
        pr_err("No syscalls specified\n");
        return -EINVAL;
    }

    syscall_names = kmalloc_array(num_syscalls, sizeof(char*), GFP_KERNEL);
    kprobes = kmalloc_array(num_syscalls, sizeof(struct kprobe), GFP_KERNEL);

    if(!syscall_names || !kprobes){
        kfree(syscall_names);
        kfree(kprobes);
        return -ENOMEM;
    }

    str = kstrdup(syscall_syms, GFP_KERNEL);
    if(!str){
        kfree(syscall_names);
        kfree(kprobes);
        return -ENOMEM;
    }

    saveptr = str;
    for (i = 0; i < num_syscalls; ) {
        token = strsep(&saveptr, ",");
        if (!token || *token == '\0') 
            continue;
            
        syscall_names[i] = kstrdup(token, GFP_KERNEL);
        if (!syscall_names[i]) {
            while (--i >= 0)
                kfree(syscall_names[i]);
            kfree(syscall_names);
            kfree(kprobes);
            kfree(str);
            return -ENOMEM;
        }
        i++;
    }
    kfree(str);

    for(i = 0; i < num_syscalls; i ++){
        kprobes[i] = (struct kprobe){
            .symbol_name = syscall_names[i],
            .pre_handler = sys_call_kprobe_pre_handler
        };
    }

    for(i = 0; i < num_syscalls; i ++){
        err = register_kprobe(&kprobes[i]);
        if(err < 0){
            pr_err("Failed to register kprobe for %s: %d\n", syscall_names[i], err);
            while(--i >= 0){
                unregister_kprobe(&kprobes[i]);
            }
            for(i = 0; i < num_syscalls; i++){
                kfree(syscall_names[i]);
            }
            kfree(syscall_names);
            kfree(kprobes);
            return err;
        }
    }

	pr_info("Monitoring %d syscalls for UID:%d\n", num_syscalls, uid);
	return 0;
}

static void __exit logsys_end(void){
	int i;
    if (kprobes) {
        for (i = 0; i < num_syscalls; i++)
            unregister_kprobe(&kprobes[i]);
        kfree(kprobes);
    }
    if (syscall_names) {
        for (i = 0; i < num_syscalls; i++)
            kfree(syscall_names[i]);
        kfree(syscall_names);
    }
    pr_info("Monitoring stopped\n");
}

module_init(logsys_start);
module_exit(logsys_end);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marcos, Julio e Victor");
MODULE_DESCRIPTION("Monitor multiple syscalls for a specific UID");

// To use this module
// sudo insmod logsyscall.ko uid=1000  syscall_syms="__x64_sys_execve,__x64_sys_kill,__x64_sys_exit_group,__x64_sys_clone"