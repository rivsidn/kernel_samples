#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/seqlock.h>
#include <linux/kthread.h>
#include <linux/delay.h>

seqlock_t lock;

static struct task_struct *task1;
static struct task_struct *task2;

int value;

/* 写操作 */
static int do_task1(void *data)
{
	msleep(100);
	write_seqlock(&lock);
	value = 20;
	write_sequnlock(&lock);

	while (1) {
		msleep(10);
		if (kthread_should_stop())
			break;
	}
	return 0;
}

/* 读操作 */
static int do_task2(void *data)
{
	unsigned seq;

	do {
		seq = read_seqbegin(&lock);
		printk("%d\n", value);
		msleep(150);
	} while (read_seqretry(&lock, seq));

	while (1) {
		msleep(10);
		if (kthread_should_stop())
			break;
	}
	return 0;
}

static int __init demo_init(void)
{
	int ret = 0;

	value = 10;
	seqlock_init(&lock);

	task1 = kthread_create(do_task1, NULL, "demo_task1");
	if (IS_ERR(task1)) {
		ret = PTR_ERR(task1);
		task1 = NULL;
		goto err;
	}
	task2 = kthread_create(do_task2, NULL, "demo_task2");
	if (IS_ERR(task2)) {
		ret = PTR_ERR(task2);
		task2 = NULL;
		goto err1;
	}

	wake_up_process(task1);
	wake_up_process(task2);

	return 0;

err1:
	kthread_stop(task1);
	task1 = NULL;
err:
	return ret;
}
      
static void __exit demo_exit(void)
{
	if (task1)
		kthread_stop(task1);
	if (task2)
		kthread_stop(task2);

	task1 = NULL;
	task2 = NULL;
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
