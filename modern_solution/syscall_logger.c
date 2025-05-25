#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <asm/special_insns.h>

MODULE_LICENSE("GPL");

static unsigned long **sys_call_table;
static asmlinkage long (*orig_read)(int fd, char __user *buf, size_t count);

// Critical: Ensure WP and preemption are ALWAYS restored
static void disable_wp(void) {
    preempt_disable();
    barrier();
    write_cr0(read_cr0() & ~X86_CR0_WP);
}

static void enable_wp(void) {
    write_cr0(read_cr0() | X86_CR0_WP);
    barrier();
    preempt_enable();
}

static noinline asmlinkage long my_read(int fd, char __user *buf, size_t count) {
    // pr_info("read() called by UID: %d\n", __kuid_val(current_uid()));
    return orig_read(fd, buf, count);
}

static int __init chs_init(void) {
    unsigned long (*kallsyms_lookup)(const char *);
    struct kprobe kp = { .symbol_name = "kallsyms_lookup_name" };
    int ret = -1;

    // Resolve kallsyms_lookup_name
    if (register_kprobe(&kp) < 0) {
        pr_err("Kprobe registration failed\n");
        return -1;
    }
    kallsyms_lookup = (unsigned long (*)(const char *))kp.addr;
    unregister_kprobe(&kp);

    // Get sys_call_table
    sys_call_table = (unsigned long **)kallsyms_lookup("sys_call_table");
    if (!sys_call_table) {
        pr_err("sys_call_table not found\n");
        return -ENOENT;
    }

    // Atomic hook
    disable_wp();
    orig_read = (void *)sys_call_table[__NR_read];
    sys_call_table[__NR_read] = (unsigned long *)my_read;
    enable_wp();

    pr_info("Module loaded successfully\n");
    return 0;
}

static void __exit chs_exit(void) {
    if (sys_call_table) {
        disable_wp();
        sys_call_table[__NR_read] = (unsigned long *)orig_read;
        enable_wp();
    }
    pr_info("Module unloaded\n");
}

module_init(chs_init);
module_exit(chs_exit);