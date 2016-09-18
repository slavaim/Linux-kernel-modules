#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/rcupdate.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Slava Imameev");

const char* state_to_string(long state)
{
	switch (state)
	{
		case TASK_RUNNING: return "TASK_RUNNING";
		case TASK_INTERRUPTIBLE: return "TASK_INTERRUPTIBLE";
		case TASK_UNINTERRUPTIBLE: return "TASK_UNINTERRUPTIBLE";
		case __TASK_STOPPED: return "__TASK_STOPPED";
		case __TASK_TRACED: return "__TASK_TRACED";
		case TASK_DEAD: return "TASK_DEAD";
		case TASK_WAKEKILL: return "TASK_WAKEKILL";
		case TASK_WAKING: return "TASK_WAKING";
		case TASK_PARKED: return "TASK_PARKED";
		case TASK_NOLOAD: return "TASK_NOLOAD";
		default: return "UNKNOWN";
	}
}

void list_from_task(struct task_struct *task)
{
	// tasklist_lock is not exported anymore, use RCU as p->tasks is updated wth list_add_tail_rcu(), see copy_process(),
	// you can use list_for_each_entry_rcu() instead the explicit RCU list traversing code below
    rcu_read_lock();
    {
    	struct task_struct* p = task;

    	do
    	{
    		struct list_head*  next;
    		long               state = p->state; // the value is volatile and will be accessed twice, make a copy for consistency

    		printk(KERN_INFO "task: %s, pid: [%d], state: %li(%s)\n", p->comm, p->pid, state, state_to_string(state));

    		next = rcu_dereference(p->tasks.next);
    		p = list_entry(next, struct task_struct, tasks);
    	}
    	while (p != task);
    }
    rcu_read_unlock();
}

void list_tasks(void)
{
	list_from_task(&init_task);
}

static int __init _init(void)
{
	printk(KERN_INFO "Hi from init\n");

	list_tasks();

	return 0;
}

static void __exit _exit(void)
{
	printk(KERN_INFO "Bye!\n");
}

module_init(_init)
module_exit(_exit)
