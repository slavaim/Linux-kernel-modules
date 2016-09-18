#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include "foo.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Slava Imameev");

static int __init _init(void)
{
	printk(KERN_INFO "Hi from init\n");
	foo();
	return 0;
}

static __exit void _exit(void)
{
	printk(KERN_INFO "Bye!\n");
}

module_init(_init)
module_exit(_exit)
