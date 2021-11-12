/*
 * 利用ftrace 挂载钩子函数。
 *
 * 编译通过内核列表:
 *
 * 参考资料:
 * https://www.kernel.org/doc/html/latest/trace/ftrace-uses.html
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ftrace.h>

static void my_callback_func(unsigned long ip, unsigned long parent_ip)
{
	return;
}

static struct ftrace_ops ops = {
	.func = my_callback_func,
};

static int __init demo_init(void)
{
	int ret;

	ret = register_ftrace_function(&ops);
	if (ret != 0) {
		printk("register ftrace function error\n");
		return ret;
	}
	return 0;
}
      
static void __exit demo_exit(void)
{
	unregister_ftrace_function(&ops);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
