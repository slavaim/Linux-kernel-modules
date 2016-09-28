#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mmu_context.h> // for use_mm()
#include <linux/sched.h> // for get_task_mm()
#include <linux/pid.h> // for find_vpid() , pid_task()
#include <linux/rwsem.h> // for down_read(), up_read()
#include <linux/mm_types.h> // for vm_area_struct
#include <asm/uaccess.h>    // for segment descriptors and copy_from_user
#include "workqueue.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Slava Imameev");

//
// as a demonstration we will attach to the init task
//
pid_t  g_pid = 1;

void attach_to_task_mm(void* ppid)
{
	pid_t                pid = *(pid_t*)ppid;
	struct task_struct*  task = NULL;
	struct mm_struct*    mm = NULL; // referenced

	//
	// the task list is protected by RCU
	//
	rcu_read_lock();
	{ // lock RCU
		task = pid_task(find_vpid(pid), PIDTYPE_PID);
		if (task)
			mm = get_task_mm(task); // take a reference to mm
	} // unlock RCU
	rcu_read_unlock();

	if (mm) {

		mm_segment_t    old_fs;

		// set the user space adress limit
		old_fs = get_fs();
		set_fs(USER_DS);
		{ // changed fs

			printk(KERN_INFO "The old user address limit is 0x%lx , the new is 0x%lx\n", old_fs.seg, get_fs().seg);

			// attach to a task address space
			use_mm(mm);
			{ // attach mm

				unsigned long bytes_not_read;
				unsigned long ul = 0;

				// read the first bytes of a code segment
				bytes_not_read = copy_from_user(&ul, (void*)mm->start_code, sizeof(ul));

				printk(KERN_INFO "read %li bytes of code, ul=0x%lx\n", (sizeof(ul)-bytes_not_read), ul);

				// iterate over all task's VMAs with the mmap semaphor being held
				down_read(&mm->mmap_sem);
				{ // lock mmap

					struct vm_area_struct*   vma;
					for (vma = mm->mmap; vma; vma = vma->vm_next) {

						printk(KERN_INFO "VMA {0x%lx,0x%lx}\n", vma->vm_start, vma->vm_end);

						// read sizeof(long) bytes from user space
						bytes_not_read = copy_from_user(&ul, (void*)vma->vm_start, sizeof(ul));

						printk(KERN_INFO "read %li bytes VMA start, ul=%lx\n", (sizeof(ul)-bytes_not_read), ul);
					}

				} // unlock mmap
				up_read(&mm->mmap_sem);
			} // detach mm
			unuse_mm(mm);
		} // restore fs
		set_fs(old_fs);

		// release the mm structure
		mmput(mm);

	} else {

		printk(KERN_INFO "a task with pid=%i was not found or doesn't have mm\n", pid);
	}
}

static int __init _init(void)
{
	int err;

	// use_mm() can be used only with kernel threads so the work queue is used
	err = wq_init();
	if (err)
		return err;

	add_entry(attach_to_task_mm, &g_pid);

	return 0;
}


static void __exit _exit(void)
{
	wq_exit();

	printk(KERN_INFO "Bye!\n");
}

module_init(_init);
module_exit(_exit);
