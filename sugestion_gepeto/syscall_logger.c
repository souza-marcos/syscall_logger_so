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

#define PROC_FILENAME "syslog_filter"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("UFMG - Júlio, Marcos, Victor");
MODULE_DESCRIPTION("Syscall Logger com Filtro por UID via /proc");

static unsigned long **syscall_table;
static int filter_uid = -1; // -1 significa sem filtro

// Protótipos originais
asmlinkage long (*original_openat)(const struct pt_regs *);
asmlinkage long (*original_execve)(const struct pt_regs *);
asmlinkage long (*original_clone)(const struct pt_regs *);

// Protótipos hooked
asmlinkage long hooked_openat(const struct pt_regs *regs);
asmlinkage long hooked_execve(const struct pt_regs *regs);
asmlinkage long hooked_clone(const struct pt_regs *regs);


// Utilitário para localizar a syscall_table (exige kallsyms exportado no kernel)
static unsigned long **find_syscall_table(void) {
    //extern unsigned long __symbol_get(const char *name);
    return (unsigned long **)__symbol_get("sys_call_table");
}

// Hook: openat (chamado por open)
asmlinkage long hooked_openat(const struct pt_regs *regs) {
    kuid_t uid = current_uid();

    if (filter_uid == -1 || __kuid_val(uid) == filter_uid) {
        char __user *filename = (char __user *)regs->si;
        char fname[256] = {0};
        if (filename && strncpy_from_user(fname, filename, sizeof(fname) - 1) > 0) {
            printk(KERN_INFO "[syslogger] UID %d chamou openat('%s')\n", __kuid_val(uid), fname);
        }
    }

    return original_openat(regs);
}

// Hook: execve
asmlinkage long hooked_execve(const struct pt_regs *regs) {
    kuid_t uid = current_uid();
    if (filter_uid == -1 || __kuid_val(uid) == filter_uid) {
        char __user *filename = (char __user *)regs->di;
        char fname[256] = {0};
        if (filename && strncpy_from_user(fname, filename, sizeof(fname) - 1) > 0) {
            printk(KERN_INFO "[syslogger] UID %d chamou execve('%s')\n", __kuid_val(uid), fname);
        }
    }

    return original_execve(regs);
}

// Hook: clone (fork/thread)
asmlinkage long hooked_clone(const struct pt_regs *regs) {
    kuid_t uid = current_uid();
    if (filter_uid == -1 || __kuid_val(uid) == filter_uid) {
        printk(KERN_INFO "[syslogger] UID %d chamou fork/clone\n", __kuid_val(uid));
    }
    return original_clone(regs);
}

// Interface /proc para configurar filtro
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos) {
    char input[16];
    if (count >= sizeof(input)) return -EINVAL;

    if (copy_from_user(input, buffer, count)) return -EFAULT;
    input[count] = '\0';

    if (kstrtoint(input, 10, &filter_uid) == 0) {
        printk(KERN_INFO "[syslogger] Filtro de UID atualizado para %d\n", filter_uid);
    }

    return count;
}

static struct proc_ops proc_fops = {
    .proc_write = proc_write,
};

static void disable_write_protection(void) {
    write_cr0(read_cr0() & (~0x10000));
}

static void enable_write_protection(void) {
    write_cr0(read_cr0() | 0x10000);
}

static int __init syslogger_init(void) {
    printk(KERN_INFO "[syslogger] Iniciando módulo\n");

    syscall_table = find_syscall_table();
    if (!syscall_table) {
        printk(KERN_ERR "[syslogger] Não foi possível localizar a syscall table\n");
        return -1;
    }

    disable_write_protection();

    original_openat = (void *)syscall_table[__NR_openat];
    original_execve = (void *)syscall_table[__NR_execve];
    original_clone  = (void *)syscall_table[__NR_clone];

    syscall_table[__NR_openat] = (unsigned long *)hooked_openat;
    syscall_table[__NR_execve] = (unsigned long *)hooked_execve;
    syscall_table[__NR_clone]  = (unsigned long *)hooked_clone;

    enable_write_protection();

    proc_create(PROC_FILENAME, 0666, NULL, &proc_fops);

    return 0;
}

static void __exit syslogger_exit(void) {
    printk(KERN_INFO "[syslogger] Removendo módulo\n");

    remove_proc_entry(PROC_FILENAME, NULL);

    if (syscall_table) {
        disable_write_protection();
        syscall_table[__NR_openat] = (unsigned long *)original_openat;
        syscall_table[__NR_execve] = (unsigned long *)original_execve;
        syscall_table[__NR_clone]  = (unsigned long *)original_clone;
        enable_write_protection();
    }
}

module_init(syslogger_init);
module_exit(syslogger_exit);
