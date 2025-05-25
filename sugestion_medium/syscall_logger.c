#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/uidgid.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/string.h>
#include <asm/paravirt.h>

#define PROC_FILENAME "syslog_filter"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("UFMG - Júlio, Marcos, Victor");
MODULE_DESCRIPTION("Syscall Logger com Filtro por UID via /proc");

static unsigned long **sys_call_table;

// The system call that will be shadowed
static asmlinkage long (*orig_read)(int fd, char __user *buf, size_t count);

// My version of the previous one
static asmlinkage long my_read(int fd, char __user *buf, size_t count);

// Function to get sys_call_table address from /proc/kallsyms
static unsigned long **get_sys_call_table_from_proc(void) {
    struct file *file;
    char buffer[128];
    loff_t pos = 0;
    int ret;
    unsigned long addr = 0;
    const char *syscall_table_str = "sys_call_table";

    file = filp_open("/proc/kallsyms", O_RDONLY, 0);
    if (IS_ERR(file)) {
        pr_err("Failed to open /proc/kallsyms\n");
        return NULL;
    }

    // I was strugling in a error of finding the sycall table, so the gpt provide to me this solution
    // Saddly the error persist, but it is something about kernel's block now 
    while ((ret = kernel_read(file, buffer, sizeof(buffer) - 1, &pos)) > 0) {
        char *line = buffer;
        char *saveptr;

        buffer[ret] = '\0';

        while ((line = strsep(&saveptr, "\n")) != NULL) {
            char *addr_str = strsep(&line, " ");
            strsep(&line, " "); // Skip type
            char *symbol = strsep(&line, " ");

            if (addr_str && symbol) {
                if (strcmp(symbol, syscall_table_str) == 0) {
                    addr = simple_strtoul(addr_str, NULL, 16);
                    goto out_close;
                }
            }
        }
    }

out_close:
    filp_close(file, NULL);

    if (!addr) {
        pr_err("sys_call_table not found in /proc/kallsyms\n");
        return NULL;
    }

    return (unsigned long **)addr;
}

// Disable write protection on page tables
static void disable_write_protection(void) {
    write_cr0(read_cr0() & ~X86_CR0_WP);
}

// Re-enable write protection on page tables
static void enable_write_protection(void) {
    write_cr0(read_cr0() | X86_CR0_WP);
}

static asmlinkage long my_read(int fd, char __user *buf, size_t count) {
    kuid_t uid = current_uid();
    pr_info("[syslogger] Hooking successful, the read was called by %d\n", __kuid_val(uid));
    return orig_read(fd, buf, count);
}

// Swapping the original calls with our versions
static void syscall_hijack(void) {
    orig_read = (void *)sys_call_table[__NR_read];
    pr_info("[syslogger] Original sys_read [%p]\n", orig_read);
    disable_write_protection();
    sys_call_table[__NR_read] = (unsigned long *)my_read;
    enable_write_protection();
    pr_info("[syslogger] Read syscall hijacked\n");
}

// Module initialization
static int __init chs_init(void) {
    printk(KERN_INFO "[syslogger] Entering %s\n", __func__);

    sys_call_table = get_sys_call_table_from_proc();
    if (!sys_call_table) {
        pr_err("[syslogger] Failed to find sys_call_table\n");
        return -ENOENT;
    }

    pr_info("[syslogger] sys_call_table found at %p\n", sys_call_table);

    syscall_hijack();
    return 0;
}

// Module exit
static void __exit chs_exit(void) {
    printk(KERN_INFO "[syslogger] Removing module\n");

    if (sys_call_table) {
        disable_write_protection();
        sys_call_table[__NR_read] = (unsigned long *)orig_read;
        enable_write_protection();
        pr_info("[syslogger] Read syscall restored\n");
    }
}

module_init(chs_init);
module_exit(chs_exit);