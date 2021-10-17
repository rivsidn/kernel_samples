/*
 * 启动参数设置示例代码
 *
 * 编译通过内核列表:
 * 4.4.155
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>

#define MAX_PARAM_LENGTH 256

static char param[MAX_PARAM_LENGTH];

#ifndef MODULE
static int __init do_setup(char *str)
{
	strlcpy(param, str, MAX_PARAM_LENGTH);
	return 1;
}
__setup("boot_param=", do_setup);
#endif

static struct timer_list timer;

static void do_timeout(unsigned long data)
{
	printk("param %s\n", param);

	timer.expires = jiffies + HZ;
	add_timer(&timer);
}

static int __init demo_init(void)
{
	setup_timer(&timer, do_timeout, 0);
	timer.expires = jiffies + HZ;
	add_timer(&timer);

	return 0;
}

static void __exit demo_exit(void)
{
	int ret = del_timer(&timer);

	printk("demo_exit %d\n", ret);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
