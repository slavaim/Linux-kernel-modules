#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>  // for kthread_run
#include <linux/wait.h>     // for wait queue
#include <linux/delay.h>    // for msleep

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Slava Imameev");

DECLARE_WAIT_QUEUE_HEAD(g_wqh);
unsigned int         g_stop = 0;
struct task_struct*  g_task = NULL;

const char* state_to_string(long state)
{
	switch (state)
	{
		case TASK_RUNNING: return "TASK_RUNNING";
		case TASK_INTERRUPTIBLE: return "TASK_INTERRUPTIBLE";
		case TASK_UNINTERRUPTIBLE: return "TASK_UNINTERRUPTIBLE";
		case TASK_DEAD: return "TASK_DEAD";
		default: return "UNKNOWN";
	}
}

int kthreadf(void *data)
{
	DECLARE_WAITQUEUE(wq, current);
	add_wait_queue(&g_wqh, &wq);

	// enter in a loop waiting on the queue untill termination event
	while (!g_stop)
	{
		printk(KERN_INFO "kthread state is TASK_INTERRUPTIBLE\n");
		set_current_state(TASK_INTERRUPTIBLE);
		// call the scheduler to release a CPU
		schedule();
		// the thread has been woken up
		printk(KERN_INFO "thread has been woken up in %s\n", state_to_string(current->state));
	}

	set_current_state(TASK_RUNNING);
	remove_wait_queue(&g_wqh, &wq);

	printk(KERN_INFO "kthread state is TASK_RUNNING\n");

	// do_exit() calls put_task_struct() by setting the task state to TASK_DEAD
	do_exit(0);
	BUG(); // do_exit never returns
}

static int __init _init(void)
{

	printk(KERN_INFO "I am here!\n");

	// start a kernel thread, kthread_run creates and starts the thread, kthread_create creates a thread but doesn't start it
	g_task = kthread_run(kthreadf, NULL, "kthread_module");
	if (g_task)
	{
		// bump the task structure reference, this is safe to do as the thread waits for a termination event and is alive,
		// if the thread exits without synchronizing with a termination event a task structure returned by kthread_run
		// might be invalid as the thread already called do_exit() in that case you should use kthread_create and wake_up_process
		get_task_struct(g_task);

		printk(KERN_INFO "kthread_create call was successful!\n");

		// wait for a thread entering into a waiting state( depends on CPU speed but 1 sec is more than enough )
		// do not do this in a production module, we do this here just to show a waiting thread wakeup after it
		// sets its state to TASK_INTERRUPTIBLE
		msleep(1000);

                // perform a spurious wakeup
		printk(KERN_INFO "wake up the thread from the init routine\n");
		wake_up(&g_wqh);
	}
	else
	{
		printk(KERN_INFO "kthread_create call failed!\n");
	}

	return 0;
}

static void __exit _exit(void)
{
	if (g_task)
	{
		g_stop = 1;

		// wake up the thread, actually kthread_stop does the same
		// but doing wake up this way doesn't hurt
		wake_up(&g_wqh);

		// wait for the thread termination
        	kthread_stop(g_task);

		printk(KERN_INFO "the terminated thread state is %s\n", state_to_string(g_task->state));

		// release the task structure
		put_task_struct(g_task);
	}
	printk(KERN_INFO "Bye!\n");
}

module_init(_init);
module_exit(_exit);

