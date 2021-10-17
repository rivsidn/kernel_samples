/*
 * 内核定时器使用代码示例
 *
 * 编译通过内核列表:
 * 2.6.35.6
 * 4.4.155
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");

static struct timer_list timer;

/*
 * 该示例中的timer 是一个全局变量，所以我们可以直接在
 * 该函数中访问该定时器。
 * 当timer 不是一个全局变量的时候，可以将变量的指针通过
 * data 传递给该函数，进而操作定时器。
 */
static void do_timeout(unsigned long data)
{
	printk("timeout ...\n");

	/*
	 * 定时器执行结束后，每次都需要重新设置超时时间，
	 * 添加定时器。
	 */
	timer.expires = jiffies + HZ;
	add_timer(&timer);
}

static int __init demo_init(void)
{
	/*
	 * 1. 设置定时器
	 * 2. 设置超时时间
	 * 3. 添加定时器
	 *
	 * setup_timer() 和 add_timer() 都是返回值为void 的函数
	 */
	setup_timer(&timer, do_timeout, 0);
	timer.expires = jiffies + HZ;
	add_timer(&timer);

	return 0;
}

static void __exit demo_exit(void)
{
	/*
	 * 返回一个挂起，还没执行的定时器，返回 1；
	 * 其他时候返回 0
	 */
	int ret = del_timer(&timer);

	printk("demo_exit %d\n", ret);
}

module_init(demo_init);
module_exit(demo_exit);

