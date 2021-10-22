/*
 * 挂载一个三层协议，收到该协议的包时，设备进入到kdb。
 *
 * 注意，需要提前执行
 * echo ttyS0 > /sys/module/kgdboc/paramters/kgdboc
 *
 * 编译通过内核列表:
 * 2.6.35.6
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/kgdb.h>

static int packet_rcv(struct sk_buff *skb, struct net_device *dev,
		      struct packet_type *pt, struct net_device *orig_dev)
{
	printk("receive packet\n");

	kgdb_schedule_breakpoint();

	return 0;
}

static struct packet_type packet_type __read_mostly = {
	.type = cpu_to_be16(0x8910),
	.func = packet_rcv,
};

static int __init demo_init(void)
{
	dev_add_pack(&packet_type);

	return 0;
}
      
static void __exit demo_exit(void)
{
	dev_remove_pack(&packet_type);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
