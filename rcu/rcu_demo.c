/*
 * 模拟net_device{} 中 ip_ptr 指针的使用，研究RCU 的作用.
 *
 * rcu_assign_pointer()
 * rcu_dereference()
 * rcu_read_lock()
 * rcu_read_unlock()
 *
 * 编译通过内核列表:
 * 2.6.35.6
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>

struct TEST {
	struct rcu_head	rcu_head;
};
struct TEST *ptest;

static struct task_struct *task1;
static struct task_struct *task2;

static int do_task1(void *data)
{
	rcu_read_lock();
#if 0
	rcu_dereference(ptest);
#endif
	/* TODO: 为什么这里用msleep() 的时候这里不行 */
	mdelay(10000);
	printk("do task1 after sleep\n");
	rcu_read_unlock();

	while (1) {
		msleep(10);
		if (kthread_should_stop())
			break;
	}
	return 0;
}

static void rcu_test_print(struct rcu_head *head)
{
	printk("rcu test print\n");
}

static int do_task2(void *data)
{
	mdelay(1000);

	ptest = NULL;
	call_rcu(&ptest->rcu_head, rcu_test_print);

	while (1) {
		msleep(10);
		if (kthread_should_stop())
			break;
	}
	return 0;
}

static int __init demo_init(void)
{
	int ret;
	/* TODO: 检测内存是否申请成功 */
	ptest = kzalloc(sizeof(*ptest), GFP_KERNEL);
#if 0
	rcu_assign_pointer(ptest, ptest);
#endif

	/* 开启两个内核线程 */
	task1 = kthread_create(do_task1, NULL, "demo_task1");
	if (IS_ERR(task1)) {
		printk("kthread err 1\n");
		ret = PTR_ERR(task1);
		task1 = NULL;
		return ret;
	}
	printk("create task 1 success\n");
	task2 = kthread_create(do_task2, NULL, "demo_task2");
	if (IS_ERR(task2)) {
		printk("kthread err 2\n");
		ret = PTR_ERR(task2);
		task2 = NULL;
		goto err;
	}
	printk("create task 2 success\n");

	wake_up_process(task1);
	wake_up_process(task2);

	return 0;
err:
	kthread_stop(task1);
	return ret;
}
      
static void __exit demo_exit(void)
{
	/* 内存释放 */
	kfree(ptest);

	/* 结束两个内核线程 */
	if (task1)
		kthread_stop(task1);
	if (task2)
		kthread_stop(task2);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
