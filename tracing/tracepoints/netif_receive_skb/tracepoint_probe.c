/*
 * tracepoint 使用示例代码
 *
 * 需要在net-traces.c 中导出符号表才能正常编译通过
 *
 * 编译通过内核列表:
 * 4.4.155
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/tcp.h>
#include <trace/events/net.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("tracepoint probe demo");

void tracepoint_probe(void * __data, struct sk_buff *skb)
{
	if (ip_hdr(skb)->protocol == IPPROTO_TCP) {
		struct tcphdr *tph = (struct tcphdr *)(ip_hdr(skb) + 1);
		if (skb->transport_header != ((unsigned char *)(ip_hdr(skb) + 1) - skb->head)) {
			printk("%s %d %ld not equal saddr %pI4, daddr %pI4, sport %04x dport %04x\n",
			       "netif_receive_skb", skb->transport_header, ((unsigned char *)(ip_hdr(skb) + 1) - skb->head),
			       &(ip_hdr(skb)->saddr), &(ip_hdr(skb)->daddr),
			       ntohs(tph->source), ntohs(tph->dest));
		} else {
			printk("%s %d %ld equal saddr %pI4, daddr %pI4, sport %04x dport %04x\n",
			       "netif_receive_skb", skb->transport_header, ((unsigned char *)(ip_hdr(skb) + 1) - skb->head),
			       &(ip_hdr(skb)->saddr), &(ip_hdr(skb)->daddr),
			       ntohs(tph->source), ntohs(tph->dest));
		}
	}

	return;
}

static int tracepoint_probe_init(void)
{
	int ret = register_trace_netif_receive_skb(tracepoint_probe, NULL);
	if (ret != 0) {
		printk("register probe error\n");
		return -1;
	}

	return 0;
}

static void tracepoint_probe_exit(void)
{
	unregister_trace_netif_receive_skb(tracepoint_probe, NULL);
	tracepoint_synchronize_unregister();
	return;
}

module_init(tracepoint_probe_init);
module_exit(tracepoint_probe_exit);

