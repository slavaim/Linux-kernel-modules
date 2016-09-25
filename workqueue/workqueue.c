#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h> // for kthread_run
#include <linux/wait.h> // for wat queue
#include <linux/slab.h> // for kmalloc

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Slava Imameev");

//
// a work queue entry
//
typedef struct _wq_entry
{
	struct _wq_entry* next;
	void (*fn)(void*);
	void* data;
} wq_entry_t;

//
// a work queue list head and tail, protected by a spinlock
//
wq_entry_t*  g_wq_head = NULL;
wq_entry_t*  g_wq_tail = NULL;

DEFINE_SPINLOCK(g_wq_lock);

//
// a wait queue for the work queue
//
DECLARE_WAIT_QUEUE_HEAD(g_wq_wait);

//
// a boolean value to signal stop event
//
int    g_stop = 0;

//
// a referenced work queue task
//
struct task_struct*    g_wq_thread = NULL;

void wq_print_string(void* string)
{
	printk( KERN_INFO "%s\n", (char*)string);
}

int add_entry(void (*fn)(void*), void* data)
{
	wq_entry_t*    new;

	new = kmalloc(sizeof(*new), GFP_KERNEL);
	if (!new)
		return -ENOMEM;

	memset(new, 0, sizeof(*new));

	new->fn = fn;
	new->data = data;
	new->next = NULL;

	printk(KERN_INFO "adding a new WQ entry\n");

	spin_lock(&g_wq_lock);
	{
		if (!g_wq_head)
			g_wq_head = new;
		else
			g_wq_tail->next = new;

		g_wq_tail = new;
	}
	spin_unlock(&g_wq_lock);

	wake_up(&g_wq_wait);

	return 0;
}

void do_workqueue(void)
{

	do
	{
		wq_entry_t*   head;

		spin_lock(&g_wq_lock);
		{
			head = g_wq_head;
			g_wq_head = NULL;
			g_wq_tail = NULL;
		}
		spin_unlock(&g_wq_lock);

		while (head)
		{
			wq_entry_t*   next;

			head->fn(head->data);

			// move to next
			next = head->next;
			kfree(head);
			head = next;
		}

	}
	while (g_wq_head);

}

int wq_kthreadf(void* not_used)
{
	DECLARE_WAITQUEUE(wq, current);

	do
	{
		int   wait;

		// flush the queue
		do_workqueue();

		// acquire the lock to not lost an event when
		// a client sneaks in between and adds a new
		// task before the thread state is set to TASK_INTERRUPTIBLE
		// so a call to wake_up() is lost
		spin_lock(&g_wq_lock);
		{
			wait = (NULL == g_wq_head);
			if (wait)
			{
				add_wait_queue(&g_wq_wait, &wq);
				set_current_state(TASK_INTERRUPTIBLE);
			}
		}
		spin_unlock(&g_wq_lock);

		if (wait)
		{
			// wait for a new event
			schedule();
			set_current_state(TASK_RUNNING); // redundant but doesn't hurt
			remove_wait_queue(&g_wq_wait, &wq);
		}
	}
	while (!g_stop);

	// clean the queue
	do_workqueue();

	printk( KERN_INFO "Exiting from the work queue thread\n");
	return 0;
}

static int __init _init(void)
{
	g_wq_thread = kthread_run(wq_kthreadf, NULL, "my_wq_thread");
	if (g_wq_thread)
	{
		printk(KERN_INFO "kthread_run(wq_kthreadf) was successful\n");

		// bump the reference, it is safe to access the task structure
		// as the thread termination is synchronized with module initialization
		// routine
		get_task_struct(g_wq_thread);
	}

	add_entry(wq_print_string, "Hi from a work queue!");
	add_entry(wq_print_string, "WQ 1");
	add_entry(wq_print_string, "WQ 2");
	add_entry(wq_print_string, "WQ 3");
	add_entry(wq_print_string, "WQ 4");
	add_entry(wq_print_string, "WQ 5");
	add_entry(wq_print_string, "WQ 6");

	return 0;
}

static void __exit _exit(void)
{
	if (g_wq_thread)
	{
		// indicate thread stopping
		g_stop = 1;

		// wake up the thread
		wake_up(&g_wq_wait);

		printk(KERN_INFO "Waiting for work queue thread termination\n");

		// wait fot the kernel thread termination
		kthread_stop(g_wq_thread);
		put_task_struct(g_wq_thread);
	}

	printk(KERN_INFO "Bye!\n");
}

module_init(_init);
module_exit(_exit);
