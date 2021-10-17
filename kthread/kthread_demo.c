#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>

static struct task_struct *demo_task;

static int do_task(void *data)
{
	while (1) {
		/* TODO: 内核中的延时操作 */
		printk("do something...\n");
	}

	return 0;
}

static int __init demo_init(void)
{
	int ret;

	demo_task = kthread_create(do_task, NULL, "%s", "demo_task");
	if (IS_ERR(demo_task)) {
		ret = ERR_PTR(demo_task);
		demo_task = NULL;
		return ret;
	}
	/* TODO: 启动之后必须要执行唤醒操作么？ */
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
