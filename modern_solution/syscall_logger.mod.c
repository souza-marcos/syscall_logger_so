#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_MITIGATION_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const char ____versions[]
__used __section("__versions") =
	"\x1c\x00\x00\x00\xca\x39\x82\x5b"
	"__x86_return_thunk\0\0"
	"\x10\x00\x00\x00\xb0\x3e\x2d\x6a"
	"pv_ops\0\0"
	"\x14\x00\x00\x00\xe6\x10\xec\xd4"
	"BUG_func\0\0\0\0"
	"\x14\x00\x00\x00\x5d\x97\x00\xe3"
	"pcpu_hot\0\0\0\0"
	"\x20\x00\x00\x00\x2c\x8a\xd8\x48"
	"__SCT__preempt_schedule\0"
	"\x18\x00\x00\x00\x6e\xa2\x66\x3f"
	"register_kprobe\0"
	"\x1c\x00\x00\x00\x1d\xe6\x10\xbb"
	"unregister_kprobe\0\0\0"
	"\x24\x00\x00\x00\xce\xce\x0e\x67"
	"__x86_indirect_thunk_rbx\0\0\0\0"
	"\x14\x00\x00\x00\xbb\x6d\xfb\xbd"
	"__fentry__\0\0"
	"\x18\x00\x00\x00\x39\x29\x8f\xad"
	"const_pcpu_hot\0\0"
	"\x10\x00\x00\x00\x7e\x3a\x2c\x12"
	"_printk\0"
	"\x24\x00\x00\x00\x97\x70\x48\x65"
	"__x86_indirect_thunk_rax\0\0\0\0"
	"\x18\x00\x00\x00\x34\x61\x23\x68"
	"module_layout\0\0\0"
	"\x00\x00\x00\x00\x00\x00\x00\x00";

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "8CB780E47E62766DD168D90");
