#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static struct task_struct *demo_task;

static int do_task(void *data)
{
	while (1) {
		msleep(10);

		printk("do something...\n");

		/*
		 * 没有stop，会导致kthread_stop()不返回，为什么？
		 */
		if (kthread_should_stop())
			break;
	}

	return 0;
}

static int __init demo_init(void)
{
	int ret;

	demo_task = kthread_create(do_task, NULL, "%s", "demo_task");
	if (IS_ERR(demo_task)) {
		ret = PTR_ERR(demo_task);
		demo_task = NULL;
		return ret;
	}

	/* TODO: 代码分析为何必须要执行唤醒操作？ */
	wake_up_process(demo_task);

	return 0;
}

static void __exit demo_exit(void)
{
	if (demo_task)
		kthread_stop(demo_task);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
