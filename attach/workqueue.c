#include <linux/kernel.h>
#include <linux/kthread.h> // for kthread_run
#include <linux/wait.h> // for wat queue
#include <linux/slab.h> // for kmalloc
#include <linux/err.h>

//
// a work queue entry
//
typedef struct _wq_entry {
	struct _wq_entry* next;
	void (*fn)(void*);
	void* data;
} wq_entry_t;

//
// a work queue list head and tail, protected by a spinlock
//
static wq_entry_t*  g_wq_head = NULL;
static wq_entry_t*  g_wq_tail = NULL;

//
// a lock to protect the work queue list
//
static DEFINE_SPINLOCK(g_wq_lock);

//
// a wait queue for the work queue
//
static DECLARE_WAIT_QUEUE_HEAD(g_wq_wait);

//
// a boolean value to signal stop event
//
static int    g_stop = 0;

//
// a referenced work queue task
//
static struct task_struct*    g_wq_thread = NULL;


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

static void do_workqueue(void)
{

	do {
		wq_entry_t*   head;

		spin_lock(&g_wq_lock);
		{
			head = g_wq_head;
			g_wq_head = NULL;
			g_wq_tail = NULL;
		}
		spin_unlock(&g_wq_lock);

		while (head) {

			wq_entry_t*   next;

			head->fn(head->data);

			// move to next
			next = head->next;
			kfree(head);
			head = next;
		}

	} while (g_wq_head);

}

static int wq_kthreadf(void* not_used)
{
	DECLARE_WAITQUEUE(wq, current);

	do {
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
			if (wait) {
				add_wait_queue(&g_wq_wait, &wq);
				set_current_state(TASK_INTERRUPTIBLE);
			}
		}
		spin_unlock(&g_wq_lock);

		if (wait) {
			// wait for a new event
			schedule();
			set_current_state(TASK_RUNNING); // redundant but doesn't hurt
			remove_wait_queue(&g_wq_wait, &wq);
		}
	} while (!g_stop);

	// clean the queue
	do_workqueue();

	printk( KERN_INFO "Exiting from the work queue thread\n");
	return 0;
}

int wq_init(void)
{
	g_wq_thread = kthread_run(wq_kthreadf, NULL, "my_wq_thread");
	if (g_wq_thread) {
		printk(KERN_INFO "kthread_run(wq_kthreadf) was successful\n");

		// bump the reference, it is safe to access the task structure
		// as the thread termination is synchronized with module initialization
		// routine
		get_task_struct(g_wq_thread);
	}

	return IS_ERR(g_wq_thread) ? PTR_ERR(g_wq_thread) : 0;
}

void wq_exit(void)
{
	if (g_wq_thread) {
		// indicate thread stopping
		g_stop = 1;

		// wake up the thread
		wake_up(&g_wq_wait);

		printk(KERN_INFO "Waiting for the work queue thread termination ...\n");

		// wait fot the kernel thread termination
		kthread_stop(g_wq_thread);
		put_task_struct(g_wq_thread);

		printk(KERN_INFO "The worqueue thread has terminated\n");
	}

}
