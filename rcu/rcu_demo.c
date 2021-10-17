/*
 * 模拟net_device{} 中 ip_ptr 指针的使用，研究RCU 的作用.
 */
#include <linux/kernel.h>
#include <linux/module.h>

static int *ptr;
static int value = 10;

/*
 * 线程1 中
 * 加RCU 锁，获取ptr 指针，调度出去一段时间之后，访问数值
 */

/*
 * 线程2 中
 * 首先调度出去一段时间，设置ptr 为空
 */

static int __init demo_init(void)
{
	/* 设置ptr */
	rcu_assign_pointer(ptr, &value);

	/* 开启两个内核线程 */



	return 0;
}
      
static void __exit demo_exit(void)
{
	/* 结束两个内核线程 */
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
