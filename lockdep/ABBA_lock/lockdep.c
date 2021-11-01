/*
 * 死锁检测功能测试代码
 *
 * TODO : 尚未编译通过
 */
#include <linux/kernel.h>
#include <linux/module.h>

static struct task_struct *softirq_task;
static struct task_struct *normal_task;

DEFINE_MUTEX(lock);

static int softirq_lock(void *data)
{
	//设置软中断标识
	//获取锁
	mutex_lock(&lock);
	mdelay(100);
	//释放锁
	mutex_unlock(&lock);
	while (1) {
		msleep(100);
		if (kthread_should_stop())
			break;
	}

	return 0;
}

static int normal_lock(void *data)
{
	//获取锁
	mutex_lock(&lock);
	mdelay(100);
	//释放锁
	mutex_unlock(&lock);
	while (1) {
		msleep(100);
		if (kthread_should_stop())
			break;
	}

	return 0;
}

static int __init demo_init(void)
{
	int ret;

	//创建两个内核线程
	softirq_task = kthread_create(softirq_lock, NULL, "softirq_lockdep");
	if (IS_ERR(softirq_task)) {
		ret = PTR_ERR(softirq_task);
		softirq_task = NULL;
		return ret;
	}
	normal_task = kthread_create(normal_lock, NULL, "normal_lockdep");
	if (IS_ERR(normal_task)) {
		ret = PTR_ERR(normal_task);
		normal_task = NULL;
		goto err;
	}

	wake_up_process(softirq_task);
	wake_up_process(normal_task);

	return 0;
err:
	kthread_stop(softirq_task);
	softirq_task = NULL;
	return ret;
}
      
static void __exit demo_exit(void)
{
	//删除两个内核线程
	if (normal_task)
		kthread_stop(normal_task);
	if (softirq_task)
		kthread_stop(softirq_task);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
