#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>  // for kthread_run
#include <linux/slab.h>     // for kmalloc
#include <linux/fs.h>       // for vfs_*
#include <asm/uaccess.h>    // for segment descriptors

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Slava Imameev");

static struct task_struct* g_task = NULL;

static ssize_t read_file(char* filename, void* buffer, size_t size, loff_t offset)
{
	struct file  *f;
	mm_segment_t old_fs;
	ssize_t      bytes_read = 0;

	f = filp_open(filename, O_RDONLY, 0);
	if (!f)
	{
		printk(KERN_INFO "filp_open failed\n");
		return -ENOENT;
	}

	// prepare for calling vfs_* functions with kernel space allocated parameters
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	{
		bytes_read = vfs_read(f, buffer, size, &offset);
	}
	set_fs(old_fs);

	filp_close(f,NULL);

	return bytes_read;
}

int kthread_read(void* data)
{
	char*        buffer = NULL;
	const size_t size = 512;
	ssize_t      bytes_read;

	buffer = kmalloc(size, GFP_KERNEL);
	if (!buffer)
		goto leave;

	bytes_read = read_file( "/etc/hosts", buffer, size, 0 );
	if (bytes_read < 0)
	{
		printk( KERN_INFO "read_file failed\n");
		goto leave;
	}

	// add a zero terminator
	buffer[ bytes_read%(size-1) ] = '\0';

	printk(KERN_INFO "bytes read %d\n", (unsigned int)bytes_read);
	printk(KERN_INFO "a first read string: %s\n", buffer);

leave:

	if (buffer)
		kfree(buffer);

	return 0; // do_exit() is called when this returns
}

static int __init _init(void)
{
	g_task = kthread_create(kthread_read, NULL, "kthread_read");
	if (!IS_ERR(g_task))
	{
		// kthread_read doesn't wait for a termination event so we need to take a reference before
		// the thread has a chance to run as the thread terminates with do_exit() that calls put_task_struct()
		get_task_struct(g_task);
		wake_up_process(g_task);
	}	
	return 0;
}

static void __exit _exit(void)
{
	// wait fot the thread termination
	if (g_task)
	{
		kthread_stop(g_task);
		put_task_struct(g_task);
	}

	printk(KERN_INFO "Bye!\n");
}

module_init(_init);
module_exit(_exit);
