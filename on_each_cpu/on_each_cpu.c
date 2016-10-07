#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/smp.h>   // for on_each_cpu

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Slava Imameev");

void smp_call_func(void *info)
{
	// a function is called with preemption disabled,
	// so it is safe to call smp_processor_id()
	printk(KERN_INFO "Running on CPU[%i]\n", smp_processor_id());
	//__asm__ __volatile ("int $3\n");
}

static int __init _init(void)
{
	printk(KERN_INFO "Hi from on_each_cpu!\n");

	on_each_cpu(smp_call_func, NULL, 1);

	return 0;
}

static void __exit _exit(void)
{
	printk(KERN_INFO "Bye!\n");
}

module_init(_init);
module_exit(_exit);
