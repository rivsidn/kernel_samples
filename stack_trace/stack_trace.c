/*
 * 保存当前调用栈测试代码
 *
 * 编译通过内核列表:
 * 2.6.35.6
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/stacktrace.h>

/* 调用栈信息输出之后，dmesg查看有20行 */
static unsigned long lockdep_init_trace_data[20];
static struct stack_trace lockdep_init_trace = {
	/* 可以保存的调用栈条目数 */
	.max_entries = ARRAY_SIZE(lockdep_init_trace_data),
	/* 存储调用栈的内存起始地址 */
	.entries = lockdep_init_trace_data,
};

static int __init demo_init(void)
{
	int i;

	for (i = 0; i < 30; i++) {
		save_stack_trace(&lockdep_init_trace);
	}
	print_stack_trace(&lockdep_init_trace, 0);

	return 0;
}
      
static void __exit demo_exit(void)
{
	printk("exit...\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
