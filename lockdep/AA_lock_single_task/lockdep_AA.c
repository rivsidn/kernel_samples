/*
 * 死锁检测功能测试代码
 *
 * 编译通过内核列表:
 * 2.6.35.6
 */
#include <linux/kernel.h>
#include <linux/module.h>

spinlock_t lock;

static int __init demo_init(void)
{
	spin_lock_init(&lock);

	spin_lock(&lock);
	spin_lock(&lock);

	return 0;
}
      
static void __exit demo_exit(void)
{
	return;
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
