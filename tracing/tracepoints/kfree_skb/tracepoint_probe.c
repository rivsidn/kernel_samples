/*
 * tracepoint 使用示例代码
 *
 * 需要在net-traces.c 中导出符号表才能正常编译通过
 *
 * 编译通过内核列表:
 * 
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <trace/events/skb.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("tracepoint probe demo");

void tracepoint_probe(void * __data, struct sk_buff *skb, void *location)
{
	printk("kfree skb tracepoint...\n");

	return;
}

static int tracepoint_probe_init(void)
{
	int ret = register_trace_kfree_skb(tracepoint_probe, NULL);
	if (ret != 0) {
		printk("register probe error\n");
		return -1;
	}

	return 0;
}

static void tracepoint_probe_exit(void)
{
	unregister_trace_kfree_skb(tracepoint_probe, NULL);
	tracepoint_synchronize_unregister();
	return;
}

module_init(tracepoint_probe_init);
module_exit(tracepoint_probe_exit);

