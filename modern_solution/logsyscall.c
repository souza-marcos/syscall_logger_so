#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("UFMG - Júlio, Marcos, Victor");
MODULE_DESCRIPTION("Syscall Hooking with Kprobes");

// Kprobes for syscalls
static struct kprobe kp_execve, kp_open, kp_kill;

// Handler for sys_execve
static int handler_pre_execve(struct kprobe *p, struct pt_regs *regs) {
    char __user *filename = (char __user *)regs->di; // Pathname (arg1)
    char buf[256];
    long ret;

    // Safely copy filename from userspace
    ret = strncpy_from_user(buf, filename, sizeof(buf) - 1);
    if (ret > 0) {
        buf[ret] = '\0';
        pr_info("execve: %s (PID: %d)\n", buf, current->pid);
    }

    return 0; // Allow syscall to proceed
}

// Handler for sys_open
static int handler_pre_open(struct kprobe *p, struct pt_regs *regs) {
    char __user *filename = (char __user *)regs->di; // Pathname (arg1)
    char buf[256];
    long ret;

    ret = strncpy_from_user(buf, filename, sizeof(buf) - 1);
    if (ret > 0) {
        buf[ret] = '\0';
        pr_info("open: %s (PID: %d)\n", buf, current->pid);
    }

    return 0;
}

// Handler for sys_kill
static int handler_pre_kill(struct kprobe *p, struct pt_regs *regs) {
    pid_t pid = (pid_t)regs->di; // PID (arg1)
    int sig = (int)regs->si;      // Signal (arg2)

    pr_info("kill: PID=%d, Signal=%d (Caller PID: %d)\n", pid, sig, current->pid);
    return 0;
}

// Register kprobes
static int __init hook_init(void) {
    // Hook sys_execve
    kp_execve.pre_handler = handler_pre_execve;
    kp_execve.symbol_name = "__x64_sys_execve"; // Symbol name varies by kernel version
    if (register_kprobe(&kp_execve)) {
        pr_err("Failed to hook execve\n");
        return -1;
    }

    // Hook sys_open
    kp_open.pre_handler = handler_pre_open;
    kp_open.symbol_name = "__x64_sys_open";
    if (register_kprobe(&kp_open)) {
        pr_err("Failed to hook open\n");
        unregister_kprobe(&kp_execve);
        return -1;
    }

    // Hook sys_kill
    kp_kill.pre_handler = handler_pre_kill;
    kp_kill.symbol_name = "__x64_sys_kill";
    if (register_kprobe(&kp_kill)) {
        pr_err("Failed to hook kill\n");
        unregister_kprobe(&kp_execve);
        unregister_kprobe(&kp_open);
        return -1;
    }

    pr_info("Kprobes registered\n");
    return 0;
}

// Unregister kprobes
static void __exit hook_exit(void) {
    unregister_kprobe(&kp_execve);
    unregister_kprobe(&kp_open);
    unregister_kprobe(&kp_kill);
    pr_info("Kprobes unregistered\n");
}

module_init(hook_init);
module_exit(hook_exit);