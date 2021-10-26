#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/icmp.h>
#include <linux/tcp.h>
#include <linux/timer.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netdevice.h>
#include <linux/jhash.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <net/ip.h>
#include <net/xfrm.h>
#include "net/kfwlog.h"
#include "net/ez_debug.h"
#include "net/ip_defend_flood.h"

static ktxt_t flood_dom[] =  {
{ .id = "drop",
  {
    [LANG_EN]= NULL,
    [LANG_ZH]= "丢弃",
    [LANG_PT]= "Descartar",
  }
},
{ .id = "UDP Flood attack",
  {
    [LANG_EN]= NULL,
    [LANG_ZH]= "UDP Flood 攻击",
    [LANG_PT]= "Ataque de inundação UDP",
  }
},
{ .id = "SYN Flood attack",
  {
    [LANG_EN]= NULL,
    [LANG_ZH]= "SYN Flood 攻击",
    [LANG_PT]= "Ataque de inundação SYN"
  }
},
{ .id = "ICMP Flood attack",
  {
    [LANG_EN]= NULL,
    [LANG_ZH]= "ICMP Flood 攻击",
    [LANG_PT]= "Ataque de inundação ICMP"
  }
},
{ .id = "IP %s will be blocked for %d seconds",
  {
	[LANG_EN]= NULL,
	[LANG_ZH]= "IP %s将被阻塞%d秒",
	[LANG_PT]= "IP %s will be blocked for %d seconds"
  }
},
//Critical
{  NULL
}
};

KTXT_DOMAIN(flood_dom);

int g_interface_flood_item_max;
atomic_t g_interface_flood_item_src_num = {0};
atomic_t g_interface_flood_item_dst_num = {0};
int g_flood_hash_size;

//老化时间，单位秒
#if 0	//delete by yuchao
static int anti_flood_item_timout  = 700;
#endif
//阻塞时间，单位秒
static int anti_flood_block_time  = 300;
struct ip_defend_flood_cfg g_dst_flood_cfg;
struct ip_defend_flood_head * g_icmpfld_hash  = NULL;
struct ip_defend_flood_head * g_udpfld_hash  = NULL;
struct ip_defend_flood_head * g_synfld_hash  = NULL;

u32 g_ip_defend_flood_log = 1;

struct ip_defend_flood_info g_ip_defend_flood;

// for proc dynamic config
#define ANTI_FLOOD_PROC_DIR			"anti_flood"
#define ANTI_FLOOD_PROC_BLOCKTIME	"block_time"
#define ANTI_FLOOD_PROC_SYN		"syn_credit"
#define ANTI_FLOOD_PROC_ICMP		"icmp_credit"
#define ANTI_FLOOD_PROC_UDP			"udp_credit"

static struct proc_dir_entry *anti_flood_proc;

#define IP_DEFEND_INTERFACE_SRC_FLOOD_FLAG(defend_flag)   ((defend_flag)&(IP_DEFEND_SRC_SYNFLOOD|     \
		IP_DEFEND_SRC_UDPFLOOD|   \
		IP_DEFEND_SRC_ICMPFLOOD))
#define IP_DEFEND_INTERFACE_DST_FLOOD_FLAG(defend_flag)   ((defend_flag)&(IP_DEFEND_DST_SYNFLOOD|     \
		IP_DEFEND_DST_UDPFLOOD|   \
		IP_DEFEND_DST_ICMPFLOOD))

#define difftime(ts1, ts0) ((long)((ts1) - (ts0)))

#define list_for_each_entry_safe_rcu(pos, n, head, member)			\
	for (pos = list_entry_rcu((head)->next, typeof(*pos), member),	\
		 n = list_entry_rcu(pos->member.next, typeof(*pos), member); \
		 &pos->member != (head);					\
		 pos = n, n = list_entry_rcu(n->member.next, typeof(*n), member))


/* uptime以秒为单位 */
static inline time_t
get_uptime(void)
{
	return jiffies/HZ;
}

void ip_defend_attack_info_out(int attack_type, const struct sk_buff *skb, unsigned long time_start, unsigned long time_end, unsigned int attack_count)
{
	const struct iphdr *iph;
	const struct ipv6hdr *ip6hdr;
	char *mod;
	char buf[256];
	char mod_array[2][16] = {"ddos_synflood", "anti_attack"};
	struct log_attack attack_log;

    memset(&attack_log, 0, sizeof(struct log_attack));

	get_str_from_macaddr(eth_hdr(skb)->h_source, attack_log.smac);
	sprintf(attack_log.stime, "%ld", time_start);
	if (time_end != 0)
		sprintf(attack_log.etime, "%ld", time_end);
	else
		sprintf(attack_log.etime, "%s", "0");

	attack_log.count = attack_count;
	strncpy(attack_log.inifname, skb->dev->name, sizeof(attack_log.inifname));

	printk("ifname: %s, attack_type %d, count %d, stime %s, etime %s\n",
			attack_log.inifname, attack_type, attack_log.count, attack_log.stime, attack_log.etime);

	if (g_devid == 3) /*netgap*/
	{
		mod = mod_array[0];
	}
	else
	{
		mod = mod_array[1];
	}

	switch (ntohs(skb->protocol)) {
	case ETH_P_IP: 
		iph = ip_hdr(skb);
		sprintf(attack_log.sip, "%u.%u.%u.%u",	NIPQUAD(iph->saddr));
		sprintf(attack_log.dip, "%u.%u.%u.%u",	NIPQUAD(iph->daddr));
		switch(iph->protocol)
		{
			case IPPROTO_UDP:
				kfwlog(mod, LOGLEVEL_WARNING, "act=%s sip=%pI4 dip=%pI4 dsp_msg=\"sip=%pI4 dip=%pI4 %s\"",
						getktxt("drop"), &iph->saddr, &iph->daddr, &iph->saddr, &iph->daddr, getktxt("UDP Flood attack"));

				break;
			case IPPROTO_TCP:
				kfwlog(mod, LOGLEVEL_WARNING, "act=%s sip=%pI4 dip=%pI4 dsp_msg=\"sip=%pI4 dip=%pI4 %s\"",
						getktxt("drop"), &iph->saddr, &iph->daddr, &iph->saddr, &iph->daddr, getktxt("SYN Flood attack"));
				
				break;
			case IPPROTO_ICMP:
				kfwlog(mod, LOGLEVEL_WARNING, "act=%s sip=%pI4 dip=%pI4 dsp_msg=\"sip=%pI4 dip=%pI4 %s\"",
						getktxt("drop"), &iph->saddr, &iph->daddr, &iph->saddr, &iph->daddr, getktxt("ICMP Flood attack"));

				break;
			default:
				break;
		}
		break;
	case ETH_P_IPV6:
		ip6hdr = ipv6_hdr(skb);
		sprintf(attack_log.sip, "%pI6c", &ip6hdr->saddr);
		sprintf(attack_log.dip, "%pI6c", &ip6hdr->daddr);
		switch(/*tproto*/ipv6_hdr(skb)->nexthdr)
		{
			case IPPROTO_UDP:
				kfwlog(mod, LOGLEVEL_WARNING, "act=%s sip=%pI6c dip=%pI6c dsp_msg=\"sip=%pI6c dip=%pI6c %s\"",
						getktxt("drop"), &ip6hdr->saddr, &ip6hdr->daddr, &ip6hdr->saddr, &ip6hdr->daddr, getktxt("UDP Flood attack"));

				break;
			case IPPROTO_TCP:
				kfwlog(mod, LOGLEVEL_WARNING, "act=%s sip=%pI6c dip=%pI6c dsp_msg=\"sip=%pI6c dip=%pI6c %s\"",
						getktxt("drop"), &ip6hdr->saddr, &ip6hdr->daddr, &ip6hdr->saddr, &ip6hdr->daddr, getktxt("SYN Flood attack"));
				
				break;
			case IPPROTO_ICMPV6:
				kfwlog(mod, LOGLEVEL_WARNING, "act=%s sip=%pI6c dip=%pI6c dsp_msg=\"sip=%pI6c dip=%pI6c %s\"",
						getktxt("drop"), &ip6hdr->saddr, &ip6hdr->daddr, &ip6hdr->saddr, &ip6hdr->daddr, getktxt("ICMP Flood attack"));

				break;
			default:
				kfwlog(mod, LOGLEVEL_WARNING, "nexthdr:%d act=%s sip=%pI6c dip=%pI6c dsp_msg=\"sip=%pI6c dip=%pI6c %s\"",
						ipv6_hdr(skb)->nexthdr,  getktxt("drop"), &ip6hdr->saddr, &ip6hdr->daddr, &ip6hdr->saddr, &ip6hdr->daddr, getktxt("ICMP Flood attack"));
				
				break;
		}
		break;
	}

	snprintf(buf, sizeof(buf), getktxt("IP %s will be blocked for %d seconds"), attack_log.sip, anti_flood_block_time);
	kfwlog(mod, LOGLEVEL_WARNING, "sip=%s dip=%s dsp_msg=\"%s\"", attack_log.sip, attack_log.dip, buf);

	return;
}

u32 flood_get_hash_key(be32 ipaddr)
{
	u32 value;

	value = jhash2(&ipaddr, sizeof(ipaddr) / sizeof(__be32), 0);

	return (value & (g_flood_hash_size - 1));
}

#if 0	//delete by yuchao
void flood_item_free(struct rcu_head *head)
{
	flood_tbl_item_t *flood_item
		= container_of(head, struct flood_tbl_item, rcu);

	kfree(flood_item);
	flood_item = NULL;
}

void ip_defend_flood_del(struct ip_defend_flood_head * flood_head, be32 startIp, be32 endIp)
{
	u32 i;
	be32 ip;
	u32 hash_v;
	flood_tbl_item_t *cur, *next;
	struct list_head *hash_head;

	for (i = ntohl(startIp); i <= ntohl(endIp); i++) {
		ip = htonl(i);
		hash_v = flood_get_hash_key(ip);
		hash_head = &(flood_head[hash_v].head);
		list_for_each_entry_safe(cur, next, hash_head, hash) {
			if (cur->item.ip == ip) {
				list_del_rcu(&(cur->hash));
				call_rcu(&(cur->rcu), flood_item_free);
				flood_head->count --;
			}
		}
	}

	return;
}

void ip_defend_flood_mod(struct ip_defend_flood_head * flood_head, be32 startIp, be32 endIp, u32 threshold)
{
	u32 i;
	be32 ip;
	u32 hash_v;
	flood_tbl_item_t *cur, *next;
	struct list_head *hash_head;

	for (i = ntohl(startIp); i <= ntohl(endIp); i++) {
		ip = htonl(i);
		hash_v = flood_get_hash_key(ip);
		hash_head = &(flood_head[hash_v].head);
		list_for_each_entry_safe(cur, next, hash_head, hash) {
			if (cur->item.ip == ip) {
				cur->item.tbf.threshold = threshold;

				break;
			}
		}
	}

	return;
}

int ip_defend_flood_add(struct ip_defend_flood_head * flood_head, be32 startIp, be32 endIp, u32 threshold)
{
	u32 i;
	be32 ip;
	u32 hash_v;
	flood_item_t     *tmp_item;
	flood_tbl_item_t *tmp_tbl_item;
	struct list_head *hash_head;

	for (i = ntohl(startIp); i <= ntohl(endIp); i++) {
		ip = htonl(i);

		tmp_tbl_item = kmalloc(sizeof(flood_tbl_item_t), GFP_KERNEL);
		if (NULL == tmp_tbl_item) {
			printk("The device is out of memory, please try later.\n");
			return -1;
		}
		memset(tmp_tbl_item, 0, sizeof(flood_tbl_item_t));
		hash_v = flood_get_hash_key(ip);
		hash_head = &(flood_head[hash_v].head);

		tmp_item = &(tmp_tbl_item->item);
		tmp_item->ip = ip;
		//tmp_item->threshold = threshold;

		/* 初始化令牌桶 */
		tmp_item->tbf.threshold = threshold;
		spin_lock_init(&(tmp_item->tbf.tbf_lock));
		tmp_item->tbf.curr_token = tmp_item->tbf.threshold;
		tmp_item->tbf.prev_jiff = jiffies;

		list_add_tail_rcu(&(tmp_tbl_item->hash), hash_head);

		flood_head->count ++;
	}

	return 0;
}

int ip_defend_flood_cfg_del(struct ip_defend_flood* cfg)
{
	if (cfg->synflood)
		ip_defend_flood_del(g_synfld_hash, g_dst_flood_cfg.start_ip, g_dst_flood_cfg.end_ip);

	if (cfg->udpflood)
		ip_defend_flood_del(g_udpfld_hash, g_dst_flood_cfg.start_ip, g_dst_flood_cfg.end_ip);

	if (cfg->icmpflood)
		ip_defend_flood_del(g_icmpfld_hash, g_dst_flood_cfg.start_ip, g_dst_flood_cfg.end_ip);

	return 0;
}

int ip_defend_flood_update(struct ip_defend_flood *cfg, struct ip_defend_flood *old_cfg)
{
	int ret = 0;
	
	/*delete the old configuration*/
	if (old_cfg->synflood)
		ip_defend_flood_del(g_synfld_hash, old_cfg->start_ip, old_cfg->end_ip);

	if (old_cfg->udpflood)
		ip_defend_flood_del(g_udpfld_hash, old_cfg->start_ip, old_cfg->end_ip);

	if (old_cfg->icmpflood)
		ip_defend_flood_del(g_icmpfld_hash, old_cfg->start_ip, old_cfg->end_ip);

	/*update the entry info*/
	g_dst_flood_cfg.start_ip = cfg->start_ip;
	g_dst_flood_cfg.end_ip = cfg->end_ip;
	g_dst_flood_cfg.synflood = cfg->synflood;
	g_dst_flood_cfg.synflood_threshold = cfg->synflood_threshold;
	g_dst_flood_cfg.udpflood = cfg->udpflood;
	g_dst_flood_cfg.udpflood_threshold = cfg->udpflood_threshold;
	g_dst_flood_cfg.icmpflood = cfg->icmpflood;
	g_dst_flood_cfg.icmpflood_threshold = cfg->icmpflood_threshold;

	/*add the new configuration*/
	if (cfg->synflood) {
		ret = ip_defend_flood_add(g_synfld_hash, cfg->start_ip, cfg->end_ip, cfg->synflood_threshold);
		if (ret)
			goto out;
	}

	if (cfg->udpflood) {
		ret = ip_defend_flood_add(g_udpfld_hash, cfg->start_ip, cfg->end_ip, cfg->udpflood_threshold);
		if (ret)
			goto out;
	}

	if (cfg->icmpflood) {
		ret = ip_defend_flood_add(g_icmpfld_hash, cfg->start_ip, cfg->end_ip, cfg->icmpflood_threshold);
		if (ret)
			goto out;
	}

out:
	return ret;
}

int ip_defend_flood_cfg_add(struct ip_defend_flood* cfg)
{
	int ret = 0;

	g_dst_flood_cfg.start_ip = cfg->start_ip;
	g_dst_flood_cfg.end_ip = cfg->end_ip;
	g_dst_flood_cfg.synflood = cfg->synflood;
	g_dst_flood_cfg.synflood_threshold = cfg->synflood_threshold;
	g_dst_flood_cfg.udpflood = cfg->udpflood;
	g_dst_flood_cfg.udpflood_threshold = cfg->udpflood_threshold;
	g_dst_flood_cfg.icmpflood = cfg->icmpflood;
	g_dst_flood_cfg.icmpflood_threshold = cfg->icmpflood_threshold;

	if (cfg->synflood) {
		ret = ip_defend_flood_add(g_synfld_hash, cfg->start_ip, cfg->end_ip, cfg->synflood_threshold);
		if (ret)
			goto out;
	}

	if (cfg->udpflood) {
		ret = ip_defend_flood_add(g_udpfld_hash, cfg->start_ip, cfg->end_ip, cfg->udpflood_threshold);
		if (ret)
			goto out;
	}

	if (cfg->icmpflood) {
		ret = ip_defend_flood_add(g_icmpfld_hash, cfg->start_ip, cfg->end_ip, cfg->icmpflood_threshold);
		if (ret)
			goto out;
	}

	return ret;

out:
	return ret;
}

int ip_defend_flood_cfg_mod(struct ip_defend_flood* cfg, struct ip_defend_flood *old_cfg)
{
	return ip_defend_flood_update(cfg, old_cfg);
}
#endif

#if 0	//delete by yuchao
void interface_flood_free(struct rcu_head *head)
{
	flood_defend_item_t *item
		= container_of(head, struct flood_defend_item, rcu);

	kfree(item);
	item = NULL;
}
#endif

void flood_hash_free(struct ip_defend_interface_flood_head * flood_hash)
{
	flood_defend_item_t *item, *next_item;
	int i;
#if 1	//modified by yuchao
	for (i = 0; i < g_flood_hash_size; i++) {
		spin_lock_bh(&(flood_hash[i].lock));
		list_for_each_entry_safe(item, next_item, &(flood_hash[i].head), list) {
			list_del(&(item->list));
			kfree(item);
		}
		spin_unlock_bh(&(flood_hash[i].lock));
	}
#else
	for (i = 0; i < g_flood_hash_size; i++) {
		spin_lock_bh(&(flood_hash[i].lock));
		list_for_each_entry_safe_rcu(item, next_item, &(flood_hash[i].head), list) {
			list_del_rcu(&(item->list));
			call_rcu(&(item->rcu), interface_flood_free);
		}
		spin_unlock_bh(&(flood_hash[i].lock));
	}
#endif

	return;
}

void ip_defend_interface_flood_kfree(void)
{
#if 0	//delete by yuchao
	del_timer(&(g_ip_defend_flood.timeout));
	if (g_ip_defend_flood.flood_dst_hash) {
		flood_hash_free(g_ip_defend_flood.flood_dst_hash);
		kfree(g_ip_defend_flood.flood_dst_hash);
		atomic_set(&g_interface_flood_item_dst_num, 0);
	}
#endif
	if (g_ip_defend_flood.flood_src_hash) {
		flood_hash_free(g_ip_defend_flood.flood_src_hash);
		kfree(g_ip_defend_flood.flood_src_hash);
		atomic_set(&g_interface_flood_item_src_num, 0);
	}

	return;
}

int ip_defend_interface_flood_set(struct ip_defend_flood *cfg) //界面修改
{
	if (cfg->synflood)
		g_ip_defend_flood.defend_cfg.defend_flag |= IP_DEFEND_DST_SYNFLOOD;
	else
		g_ip_defend_flood.defend_cfg.defend_flag &= ~IP_DEFEND_DST_SYNFLOOD;

	if (cfg->udpflood)
		g_ip_defend_flood.defend_cfg.defend_flag |= IP_DEFEND_DST_UDPFLOOD;
	else
		g_ip_defend_flood.defend_cfg.defend_flag &= ~IP_DEFEND_DST_UDPFLOOD;

	if (cfg->icmpflood)
		g_ip_defend_flood.defend_cfg.defend_flag |= IP_DEFEND_DST_ICMPFLOOD;
	else
		g_ip_defend_flood.defend_cfg.defend_flag &= ~IP_DEFEND_DST_ICMPFLOOD;

	if (cfg->src_synflood)
		g_ip_defend_flood.defend_cfg.defend_flag |= IP_DEFEND_SRC_SYNFLOOD;
	else
		g_ip_defend_flood.defend_cfg.defend_flag &= ~IP_DEFEND_SRC_SYNFLOOD;

	if (cfg->src_udpflood)
		g_ip_defend_flood.defend_cfg.defend_flag |= IP_DEFEND_SRC_UDPFLOOD;
	else
		g_ip_defend_flood.defend_cfg.defend_flag &= ~IP_DEFEND_SRC_UDPFLOOD;

	if (cfg->src_icmpflood)
		g_ip_defend_flood.defend_cfg.defend_flag |= IP_DEFEND_SRC_ICMPFLOOD;
	else
		g_ip_defend_flood.defend_cfg.defend_flag &= ~IP_DEFEND_SRC_ICMPFLOOD;

	g_ip_defend_flood.defend_cfg.dst_synflood_threshold = cfg->synflood_threshold;
	g_ip_defend_flood.defend_cfg.dst_udpflood_threshold = cfg->udpflood_threshold;
	g_ip_defend_flood.defend_cfg.dst_icmpflood_threshold = cfg->icmpflood_threshold;
	g_ip_defend_flood.defend_cfg.src_synflood_threshold = cfg->src_synflood_threshold;
	g_ip_defend_flood.defend_cfg.src_udpflood_threshold = cfg->src_udpflood_threshold;
	g_ip_defend_flood.defend_cfg.src_icmpflood_threshold = cfg->src_icmpflood_threshold;

	return 0;
}

u32 flood_get_token(struct flood_tbf *curr_state, u32 *send_log, u32* attack_count)
{
	*send_log = 0;
	spin_lock(&(curr_state->tbf_lock));

	if (curr_state->curr_token > 0) {
		curr_state->curr_token--;
	} else {
		if ((jiffies - curr_state->prev_jiff) > HZ) {
			curr_state->curr_token = curr_state->threshold - 1; /* 当前报文直接走掉消耗1令牌 */
			curr_state->prev_jiff = jiffies;
		} else {
			if (curr_state->attack_state == 0) {
				curr_state->attack_count = 1;
				curr_state->attack_start_time = get_uptime();
				curr_state->attack_end_time = get_uptime();
				curr_state->attack_state = 1;
				*send_log = 1;
				*attack_count = curr_state->attack_count;
			}/* else {
				curr_state->attack_count ++;
				curr_state->attack_end_time = get_uptime();
			}*/
			spin_unlock(&(curr_state->tbf_lock));
			return FLOOD_GET_NO_TOKEN;
		}
	}

	/*if (curr_state->attack_state == 1) {
		if (difftime(get_uptime(), curr_state->attack_start_time) > 2) {
			curr_state->attack_state = 0;
			*send_log = 1;
			*attack_count = curr_state->attack_count;
		}
	}*/
	spin_unlock(&(curr_state->tbf_lock));

	return FLOOD_GET_TOKEN;

}

struct flood_defend_item *find_flood_item_by_ip(const struct net_device *dev, struct ip_defend_interface_flood_head *hash_head, be32 ip, u32 basic_src_ip)
{
#if 1	//modified by yuchao
	flood_defend_item_t *item, *flood_item;

	list_for_each_entry(item, &(hash_head->head), list) {
		if (item == NULL) {
			printk("yuchao NULL error, %d\n");
			return NULL;
		}
		if (ip == item->ip) {
			return item;
		}
	}

	if ((atomic_read(&g_interface_flood_item_src_num) + atomic_read(&g_interface_flood_item_dst_num)) > g_interface_flood_item_max) {
		return NULL;
	}

	item = kmalloc(sizeof(flood_defend_item_t), GFP_ATOMIC);
	if (NULL == item) {
		return NULL;
	}

	memset(item, 0, sizeof(flood_defend_item_t));
	item->ip = ip;
	list_for_each_entry(flood_item, &(hash_head->head), list) {
		if (ip == flood_item->ip) {
			kfree(item);
			return flood_item;
		}
	}

	list_add_tail(&(item->list), &(hash_head->head));
	if (basic_src_ip)
		atomic_inc(&g_interface_flood_item_src_num);
	else
		atomic_inc(&g_interface_flood_item_dst_num);

#else
	flood_defend_item_t *item, *flood_item;

	list_for_each_entry_rcu(item, &(hash_head->head), list) {
		if (ip == item->ip) {
			return item;
		}
	}

	if ((atomic_read(&g_interface_flood_item_src_num) + atomic_read(&g_interface_flood_item_dst_num)) > g_interface_flood_item_max) {
		return NULL;
	}

	item = kmalloc(sizeof(flood_defend_item_t), GFP_KERNEL);
	if (NULL == item) {
		return NULL;
	}

	memset(item, 0, sizeof(flood_defend_item_t));
	item->ip = ip;
	spin_lock_bh(&hash_head->lock);
	list_for_each_entry_rcu(flood_item, &(hash_head->head), list) {
		if (ip == flood_item->ip) {
			spin_unlock_bh(&hash_head->lock);
			kfree(item);
			return flood_item;
		}
	}

	list_add_tail_rcu(&(item->list), &(hash_head->head));
	if (basic_src_ip)
		atomic_inc(&g_interface_flood_item_src_num);
	else
		atomic_inc(&g_interface_flood_item_dst_num);

	spin_unlock_bh(&hash_head->lock);
#endif

	return item;
}

int ip_defend_interface_flood_proc(struct sk_buff *skb, flood_defend_item_t *tbl_item, struct flood_defend* flood_item, u32 attack_type)
{
	u32 get_token_flag;
	u32 send_log = 0, attack_count = 0;
	struct flood_tbf tbf;
	int ret = NF_ACCEPT;
	const struct iphdr *iph;

	get_token_flag	= flood_get_token(&(flood_item->tbf), &send_log, &attack_count);
	if (FLOOD_GET_NO_TOKEN == get_token_flag) {
		ret = NF_DROP;
		iph = ip_hdr(skb);
		//printk("flood_get_token srcip %pI4 no token\n", &iph->saddr);
	}

	if (send_log && g_ip_defend_flood_log) {
		tbf = flood_item->tbf;
		ip_defend_attack_info_out(attack_type, skb, tbf.attack_start_time, tbf.attack_end_time, tbf.attack_count);
		// reset tbl_item
		memset(flood_item, 0, sizeof(flood_defend_item_t));
		tbl_item->ip = iph->saddr;
		tbl_item->attack_timer = jiffies;
		tbl_item->match_timer = jiffies;
	}

	return ret;
}

u32 ip_defend_interface_synfld_proc(struct sk_buff *skb, const struct net_device* dev, struct iphdr *iph, flood_defend_item_t *tbl_item, u16 flag)
{
	int ret = NF_ACCEPT;
	struct tcphdr *tcph = (struct tcphdr *)((u8 *)iph + iph->ihl * 4);

	if (!tcph || !(tcph->syn) || (tcph->syn && tcph->ack)) {
		return NF_ACCEPT;
	}

	if (!(tbl_item->defend_flag & flag)) { //初始化
		tbl_item->defend_flag |= flag;
		spin_lock_init(&(tbl_item->synflood.tbf.tbf_lock));
		tbl_item->synflood.tbf.prev_jiff = jiffies;
		if (flag == IP_DEFEND_SRC_SYNFLOOD) {
			tbl_item->synflood.tbf.threshold = g_ip_defend_flood.defend_cfg.src_synflood_threshold;
			tbl_item->synflood.tbf.curr_token = g_ip_defend_flood.defend_cfg.src_synflood_threshold;
		} else {
			tbl_item->synflood.tbf.threshold = g_ip_defend_flood.defend_cfg.dst_synflood_threshold;
			tbl_item->synflood.tbf.curr_token = g_ip_defend_flood.defend_cfg.dst_synflood_threshold;
		}
	}

	tbl_item->match_timer = jiffies;

	ret = ip_defend_interface_flood_proc(skb, tbl_item, &(tbl_item->synflood), synflood);

	return ret;
}

u32 ip_defend_interface_icmpfld_proc(struct sk_buff *skb, const struct net_device* dev, struct iphdr *iph, flood_defend_item_t *tbl_item, u16 flag)
{
	int ret = NF_ACCEPT;
	if (!(tbl_item->defend_flag & flag)) { //初始化
		tbl_item->defend_flag |= flag;
		spin_lock_init(&(tbl_item->icmpflood.tbf.tbf_lock));
		tbl_item->icmpflood.tbf.prev_jiff = jiffies;
		if (flag == IP_DEFEND_SRC_ICMPFLOOD) {
			tbl_item->icmpflood.tbf.threshold = g_ip_defend_flood.defend_cfg.src_icmpflood_threshold;
			tbl_item->icmpflood.tbf.curr_token = g_ip_defend_flood.defend_cfg.src_icmpflood_threshold;
		} else {
			tbl_item->icmpflood.tbf.threshold = g_ip_defend_flood.defend_cfg.dst_icmpflood_threshold;
			tbl_item->icmpflood.tbf.curr_token = g_ip_defend_flood.defend_cfg.dst_icmpflood_threshold;
		}
	}

	tbl_item->match_timer = jiffies;

	ret = ip_defend_interface_flood_proc(skb, tbl_item, &(tbl_item->icmpflood), icmpflood);

	return ret;
}

u32 ip_defend_interface_udpfld_proc(struct sk_buff *skb, const struct net_device* dev, struct iphdr *iph, flood_defend_item_t *tbl_item, u16 flag)
{
	int ret = NF_ACCEPT;
	if (!(tbl_item->defend_flag & flag)) { //初始化
		tbl_item->defend_flag |= flag;
		spin_lock_init(&(tbl_item->udpflood.tbf.tbf_lock));
		tbl_item->udpflood.tbf.prev_jiff = jiffies;
		if (flag == IP_DEFEND_SRC_UDPFLOOD) {
			tbl_item->udpflood.tbf.threshold = g_ip_defend_flood.defend_cfg.src_udpflood_threshold;
			tbl_item->udpflood.tbf.curr_token = g_ip_defend_flood.defend_cfg.src_udpflood_threshold;
		} else {
			tbl_item->udpflood.tbf.threshold = g_ip_defend_flood.defend_cfg.dst_udpflood_threshold;
			tbl_item->udpflood.tbf.curr_token = g_ip_defend_flood.defend_cfg.dst_udpflood_threshold;
		}
	}

	tbl_item->match_timer = jiffies;

	ret = ip_defend_interface_flood_proc(skb, tbl_item, &(tbl_item->udpflood), udpflood);

	return ret;
}

u32 ip_defend_flood_src_proc(void *priv, struct sk_buff *skb, const struct nf_hook_state *nhs)
{
	struct iphdr *iph = ip_hdr(skb);

	flood_defend_item_t *tbl_item;
	struct ip_defend_interface_flood_head* hash_head;
	u32 hash_v;
	int ret = NF_ACCEPT;
	be32 srcIp;

	if (!nhs->in || !(IP_DEFEND_INTERFACE_SRC_FLOOD_FLAG(g_ip_defend_flood.defend_cfg.defend_flag))) {
		//printk("[%s %d] flag %02X\n", __func__, __LINE__, g_ip_defend_flood.defend_cfg.defend_flag);
		return ret;
	}
	
	srcIp = iph->saddr;
	hash_v = flood_get_hash_key(srcIp);
	hash_head = &((g_ip_defend_flood.flood_src_hash)[hash_v]);
	if (!hash_head) {
		printk("[%s %d] src_ip:%u.%u.%u.%u, hash_v %u, hash_head is null\n",
				__func__, __LINE__, NIPQUAD(srcIp), hash_v);
		return NF_ACCEPT;
	}

	spin_lock(&hash_head->lock);
	tbl_item = find_flood_item_by_ip(nhs->in, hash_head, srcIp, 1);
	if (!tbl_item) {
		spin_unlock(&hash_head->lock);
		return NF_ACCEPT;
	}

#if 0 // only for test 172.31.3.112
		if (ntohl(iph->saddr) == 2887713648L) {
			printk("src_ip:%u.%u.%u.%u, attack_state %d, curr_token %d, attack_count %d\n",
				NIPQUAD(iph->saddr),
				tbl_item->udpflood.tbf.attack_state,
				tbl_item->udpflood.tbf.curr_token,
				tbl_item->udpflood.tbf.attack_count);
			printk("attack_timer %lu, curr_time %lu, block_time %d\n",
				tbl_item->attack_timer/HZ, jiffies/HZ, anti_flood_block_time);
		}
#endif

	if (tbl_item->attack_timer && time_before(jiffies, tbl_item->attack_timer + anti_flood_block_time * HZ)) {
		//printk("src drop the packet in block_time left %lu seconds, src_ip:%u.%u.%u.%u, dst_ip:%u.%u.%u.%u\n",
		//		(tbl_item->attack_timer + anti_flood_block_time * HZ - jiffies) / HZ, NIPQUAD(iph->saddr), NIPQUAD(iph->daddr));
		spin_unlock(&hash_head->lock);
		return NF_DROP;
	}

	switch (iph->protocol) {
	case IPPROTO_TCP:
		if (g_ip_defend_flood.defend_cfg.defend_flag & IP_DEFEND_SRC_SYNFLOOD)
			ret = ip_defend_interface_synfld_proc(skb, nhs->in, iph, tbl_item, IP_DEFEND_SRC_SYNFLOOD);

		break;
	case IPPROTO_UDP:
		if (g_ip_defend_flood.defend_cfg.defend_flag & IP_DEFEND_SRC_UDPFLOOD)
			ret = ip_defend_interface_udpfld_proc(skb, nhs->in, iph, tbl_item, IP_DEFEND_SRC_UDPFLOOD);
		
		break;
	case IPPROTO_ICMP:
		if (g_ip_defend_flood.defend_cfg.defend_flag & IP_DEFEND_SRC_ICMPFLOOD)
			ret = ip_defend_interface_icmpfld_proc(skb, nhs->in, iph, tbl_item, IP_DEFEND_SRC_ICMPFLOOD);
		
		break;
	default:
		break;
	}
	spin_unlock(&hash_head->lock);
	if (ret == NF_DROP) {
		printk("ip defend interface flood basic src drop the packet, src_ip:%u.%u.%u.%u, dst_ip:%u.%u.%u.%u\n", NIPQUAD(iph->saddr), NIPQUAD(iph->daddr));
	}

	return ret;
}

#if 0	//delete by yuchao
u32 ip_defend_synflood_dst_proc(struct sk_buff *skb)
{
	struct iphdr *iph = ip_hdr(skb);
	struct net_device*in = skb->dev;
	flood_defend_item_t *tbl_item;
	struct ip_defend_interface_flood_head* hash_head;
	u32 hash_v;
	be32 dstIp;

	if (!in || !(g_ip_defend_flood.defend_cfg.defend_flag & IP_DEFEND_DST_SYNFLOOD)) {
		return NF_ACCEPT;
	}

	dstIp = iph->daddr;
	hash_v = flood_get_hash_key(dstIp);
	hash_head = &(g_ip_defend_flood.flood_dst_hash[hash_v]);

	tbl_item = find_flood_item_by_ip(in, hash_head, dstIp, 0);
	if (!tbl_item) {
		return NF_ACCEPT;
	}

	return ip_defend_interface_synfld_proc(skb, in, iph, tbl_item, IP_DEFEND_DST_SYNFLOOD);
}

u32 ip_defend_udpflood_dst_proc(struct sk_buff *skb)
{
	struct iphdr *iph = ip_hdr(skb);
	struct net_device*in = skb->dev;
	flood_defend_item_t *tbl_item;
	struct ip_defend_interface_flood_head* hash_head;
	u32 hash_v;
	be32 dstIp;

	if (!in || !(g_ip_defend_flood.defend_cfg.defend_flag & IP_DEFEND_DST_UDPFLOOD)) {
		return NF_ACCEPT;
	}

	dstIp = iph->daddr;
	hash_v = flood_get_hash_key(dstIp);
	hash_head = &(g_ip_defend_flood.flood_dst_hash[hash_v]);

	tbl_item = find_flood_item_by_ip(in, hash_head, dstIp, 0);
	if (!tbl_item) {
		return NF_ACCEPT;
	}

	return ip_defend_interface_udpfld_proc(skb, in, iph, tbl_item, IP_DEFEND_DST_UDPFLOOD);
}

u32 ip_defend_icmpflood_dst_proc(struct sk_buff *skb)
{
	struct iphdr *iph = ip_hdr(skb);
	struct net_device*in = skb->dev;
	flood_defend_item_t *tbl_item;
	struct ip_defend_interface_flood_head* hash_head;
	u32 hash_v;
	be32 dstIp;

	if (!in || !(g_ip_defend_flood.defend_cfg.defend_flag & IP_DEFEND_DST_ICMPFLOOD)) {
		return NF_ACCEPT;
	}

	dstIp = iph->daddr;
	hash_v = flood_get_hash_key(dstIp);
	hash_head = &(g_ip_defend_flood.flood_dst_hash[hash_v]);

	tbl_item = find_flood_item_by_ip(in, hash_head, dstIp, 0);
	if (!tbl_item) {
		return NF_ACCEPT;
	}

	return ip_defend_interface_icmpfld_proc(skb, in, iph, tbl_item, IP_DEFEND_DST_ICMPFLOOD);
}

//基于目的地址的flood攻击处理
u32 flood_proc(struct sk_buff *skb, struct list_head *hash_head, be32 dstIp, enum ip_attack_type attack_type)
{
	flood_tbl_item_t *tbl_item;
	flood_item_t   *item;
	u32 get_token_flag;
	u32 ret = NF_ACCEPT;
	u32 send_log = 0, attack_count = 0;
	struct flood_tbf tbf;

	//查找目的防护配置，如果有的话按目的防护的配置进行处理
	list_for_each_entry_rcu(tbl_item, hash_head, hash) {
		item = &(tbl_item->item);
		if (item->ip == dstIp) {
			/* 获取令牌 */
			get_token_flag	= flood_get_token(&tbl_item->item.tbf, &send_log, &attack_count);
			if (FLOOD_GET_NO_TOKEN == get_token_flag) {
				ret = NF_DROP;
			}

			if (send_log && g_ip_defend_flood_log) {
				tbf = tbl_item->item.tbf;
				ip_defend_attack_info_out(attack_type, skb, tbf.attack_start_time, tbf.attack_end_time, tbf.attack_count);
			}

			return ret;
		}
	}

	//没有配置目的防护列表，则根据入接口上配置的相应攻击作处理

	switch (attack_type) {
	case icmpflood:
		ret = ip_defend_icmpflood_dst_proc(skb);
		break;
	case udpflood:
		ret = ip_defend_udpflood_dst_proc(skb);
		break;
	case synflood:
		ret = ip_defend_synflood_dst_proc(skb);
		break;
	default:
		ret = NF_ACCEPT;
	}

	return  ret;
}
u32 ip_defend_icmpfld_proc(struct sk_buff *skb, struct iphdr * iph)
{
	struct list_head *hash_head;
	u32 hash_v;
	be32 dstIp;

	if (g_icmpfld_hash->count == 0) {
		return  ip_defend_icmpflood_dst_proc(skb);
	}

	dstIp = iph->daddr;

	hash_v = flood_get_hash_key(dstIp);
	hash_head = &(g_icmpfld_hash[hash_v].head);

	return flood_proc(skb, hash_head, dstIp, icmpflood);
}

u32 ip_defend_udpfld_proc(struct sk_buff *skb, struct iphdr * iph)
{
	struct list_head *hash_head;
	u32 hash_v;
	be32 dstIp;

	if (g_udpfld_hash->count == 0) {
		return  ip_defend_udpflood_dst_proc(skb);
	}

	dstIp = iph->daddr;

	hash_v = flood_get_hash_key(dstIp);
	hash_head = &(g_udpfld_hash[hash_v].head);

	return flood_proc(skb, hash_head, dstIp, udpflood);
}

u32 ip_defend_synfld_proc(struct sk_buff *skb, struct iphdr * iph)
{
	struct list_head *hash_head;
	struct tcphdr *tcph = (struct tcphdr *)((u8 *)iph + iph->ihl * 4);
	u32 hash_v;
	be32 dstIp;

	if (g_synfld_hash->count == 0) {
		return  ip_defend_synflood_dst_proc(skb);  //基于接口的synflood攻击
	}

	if (!tcph || !(tcph->syn) || (tcph->syn && tcph->ack)) {
		return NF_ACCEPT;
	}

	dstIp = iph->daddr;

	hash_v = flood_get_hash_key(dstIp);
	hash_head = &(g_synfld_hash[hash_v].head);

	return flood_proc(skb, hash_head, dstIp, synflood);
}

u32 ip_defend_flood_dst_proc(void *priv, struct sk_buff *skb, const struct nf_hook_state *nhs)
{
	struct iphdr *iph = ip_hdr(skb);
	int ret = NF_ACCEPT;

	switch (iph->protocol) {
	case IPPROTO_TCP:
		ret = ip_defend_synfld_proc(skb, iph);
		break;
	case IPPROTO_UDP:
		ret = ip_defend_udpfld_proc(skb, iph);
		break;
	case IPPROTO_ICMP:
		ret = ip_defend_icmpfld_proc(skb, iph);
		break;
	default:
		break;
	}

	if (ret == NF_DROP) {
		printk("ip defend flood basic dst drop the packet, src_ip:%u.%u.%u.%u, dst_ip:%u.%u.%u.%u\n",
				NIPQUAD(iph->saddr), NIPQUAD(iph->daddr));
	}

	return ret;
}
#endif

static int anti_proc_show(struct seq_file *seq, void *offset)
{
	int enable = 0, threshold = 0;
	void *v = seq->private;
	struct ip_defend_interface_flood_cfg *cfg;

	if (v == &anti_flood_block_time) {
		seq_printf(seq, "%d\n", *(int *)v);
		//printk("anti_proc_show file %s, data %d\n", seq->file->f_path.dentry->d_iname, *(int *)v);
	} else if (v == &g_ip_defend_flood.defend_cfg) {
		cfg = (struct ip_defend_interface_flood_cfg *)v;
		if (!strcmp(seq->file->f_path.dentry->d_iname, ANTI_FLOOD_PROC_SYN)) {
			enable = (g_ip_defend_flood.defend_cfg.defend_flag & IP_DEFEND_SRC_SYNFLOOD) ? 1 : 0;
			threshold = g_ip_defend_flood.defend_cfg.src_synflood_threshold;
		} else if (!strcmp(seq->file->f_path.dentry->d_iname, ANTI_FLOOD_PROC_UDP)) {
			enable = g_ip_defend_flood.defend_cfg.defend_flag & IP_DEFEND_SRC_UDPFLOOD ? 1 : 0;
			threshold = g_ip_defend_flood.defend_cfg.src_udpflood_threshold;
		} else if (!strcmp(seq->file->f_path.dentry->d_iname, ANTI_FLOOD_PROC_ICMP)) {
			enable = g_ip_defend_flood.defend_cfg.defend_flag & IP_DEFEND_SRC_ICMPFLOOD ? 1 : 0;
			threshold = g_ip_defend_flood.defend_cfg.src_icmpflood_threshold;
		}
		seq_printf(seq, "%d %d\n", enable, threshold);
		//printk("anti_proc_show file %s, data %d %d\n", seq->file->f_path.dentry->d_iname, enable, threshold);
	} else {
		return -1;
	}

	return 0;
}

static int anti_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, anti_proc_show, PDE_DATA(inode));
}

static ssize_t anti_proc_write(struct file *file, const char __user *buffer,
			       size_t count, loff_t *pos)
{
	int  ret;
	int  block, enable, threshold;
	char kbuff[64];
	char *head;
	int  *pvalue = (int *)((struct seq_file *)file->private_data)->private;

	memset(kbuff, 0, sizeof(kbuff));
	if (count > sizeof(kbuff)) {
		printk("[%s %d]\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (copy_from_user(kbuff, buffer, count)) {
		printk("[%s %d]\n", __func__, __LINE__);
		return -EFAULT;
	}
	//printk("[%s %d] file %s, buf %s\n", __func__, __LINE__, file->f_path.dentry->d_iname, kbuff);
	if (pvalue == &anti_flood_block_time) {
		ret = kstrtoint(kbuff, 0, &block);
		if (ret) {
			printk("[%s %d]\n", __func__, __LINE__);
			return -EINVAL;
		}
		/* TODO: 此处需要明确范围 */
		if (block >= 10 && block <= 600)
			*pvalue = block;
		else {
			printk("[%s %d] block time %d out of range\n", __func__, __LINE__, block);
			return -EINVAL;
		}
	} else if (pvalue == (int *)&g_ip_defend_flood.defend_cfg) {
		// eg: 1 100
		head = kbuff;
		head[1] = '\0';
		ret = kstrtoint(head, 0, &enable);
		if (ret) {
			printk("[%s %d] head %s\n", __func__, __LINE__, head);
			return -EINVAL;
		}

		head += 2;
		ret = kstrtoint(head, 0, &threshold);
		if (ret) {
			printk("[%s %d] head %s\n", __func__, __LINE__, head);
			return -EINVAL;
		}
		//printk("[%s %d] file %s, enable %d, hold %d\n", __func__, __LINE__,
		//		file->f_path.dentry->d_iname, enable, threshold);
		if (threshold >= 100 && threshold <= 10000) {
			if (!strcmp(file->f_path.dentry->d_iname, ANTI_FLOOD_PROC_SYN)) {
				if (enable)
					g_ip_defend_flood.defend_cfg.defend_flag |= IP_DEFEND_SRC_SYNFLOOD;
				else
					g_ip_defend_flood.defend_cfg.defend_flag &= ~IP_DEFEND_SRC_SYNFLOOD;

				g_ip_defend_flood.defend_cfg.src_synflood_threshold = threshold;
			} else if (!strcmp(file->f_path.dentry->d_iname, ANTI_FLOOD_PROC_UDP)) {
				if (enable)
					g_ip_defend_flood.defend_cfg.defend_flag |= IP_DEFEND_SRC_UDPFLOOD;
				else
					g_ip_defend_flood.defend_cfg.defend_flag &= ~IP_DEFEND_SRC_UDPFLOOD;

				g_ip_defend_flood.defend_cfg.src_udpflood_threshold = threshold;
			} else if (!strcmp(file->f_path.dentry->d_iname, ANTI_FLOOD_PROC_ICMP)) {
				if (enable)
					g_ip_defend_flood.defend_cfg.defend_flag |= IP_DEFEND_SRC_ICMPFLOOD;
				else
					g_ip_defend_flood.defend_cfg.defend_flag &= ~IP_DEFEND_SRC_ICMPFLOOD;

				g_ip_defend_flood.defend_cfg.src_icmpflood_threshold = threshold;
			}
		}
		else {
			printk("[%s %d] file %s, threshold %d out of range\n", __func__, __LINE__,
					file->f_path.dentry->d_iname, threshold);
			return -EINVAL;
		}
	} else {
		return -1;
	}

	return count;
}

static struct file_operations anti_proc_ops = {
	.owner	=	THIS_MODULE,
	.open	=	anti_proc_open,
	.write	=	anti_proc_write,
	.read	=	seq_read,
	.llseek	=	seq_lseek,
	.release=	single_release,
};

static int anti_flood_proc_init(void)
{
	struct proc_dir_entry *tmp;

	//创建目录anti_flood
	anti_flood_proc = proc_mkdir(ANTI_FLOOD_PROC_DIR, proc_leadsec);
	if (!anti_flood_proc) {
		goto out;
	}
	//创建文件阻塞时间
	tmp = proc_create_data(ANTI_FLOOD_PROC_BLOCKTIME, 0600, anti_flood_proc,
			       &anti_proc_ops, &anti_flood_block_time);
	if (!tmp) {
		goto out1;
	}
	//创建文件sync
	tmp = proc_create_data(ANTI_FLOOD_PROC_SYN, 0600, anti_flood_proc,
			       &anti_proc_ops, &g_ip_defend_flood.defend_cfg);
	if (!tmp) {
		goto out2;
	}
	//创建文件UDP
	tmp = proc_create_data(ANTI_FLOOD_PROC_UDP, 0600, anti_flood_proc,
			       &anti_proc_ops, &g_ip_defend_flood.defend_cfg);
	if (!tmp) {
		goto out3;
	}
	//创建文件icmp
	tmp = proc_create_data(ANTI_FLOOD_PROC_ICMP, 0600, anti_flood_proc,
			       &anti_proc_ops, &g_ip_defend_flood.defend_cfg);
	if (!tmp) {
		goto out4;
	}

	return 0;

out4:
	remove_proc_entry(ANTI_FLOOD_PROC_UDP, anti_flood_proc);
out3:
	remove_proc_entry(ANTI_FLOOD_PROC_SYN, anti_flood_proc);
out2:
	remove_proc_entry(ANTI_FLOOD_PROC_BLOCKTIME, anti_flood_proc);
out1:
	remove_proc_entry(ANTI_FLOOD_PROC_DIR, proc_leadsec);
out:
	return -1;
}

static void anti_flood_proc_exit(void)
{
	//删除文件icmp
	remove_proc_entry(ANTI_FLOOD_PROC_ICMP, anti_flood_proc);
	//删除文件UDP
	remove_proc_entry(ANTI_FLOOD_PROC_UDP, anti_flood_proc);
	//删除文件syn
	remove_proc_entry(ANTI_FLOOD_PROC_SYN, anti_flood_proc);
	//删除文件阻塞时间
	remove_proc_entry(ANTI_FLOOD_PROC_BLOCKTIME, anti_flood_proc);
	//删除目录anti_flood
	remove_proc_entry(ANTI_FLOOD_PROC_DIR, proc_leadsec);
}

/*static struct nf_hook_ops ip_defend_flood_dst_ops[] __read_mostly = {
	{
		.hook		= ip_defend_flood_dst_proc,
		.pf 	= NFPROTO_IPV4,
		.hooknum	= NF_INET_PRE_ROUTING,
		.priority	= NF_IP_PRI_IP_DEFEND_FLOOD,
	},
}; */

static struct nf_hook_ops ip_defend_flood_src_ops[] __read_mostly = {
	{
		.hook		= ip_defend_flood_src_proc,
		.pf 		= NFPROTO_IPV4,
		.hooknum	= NF_INET_PRE_ROUTING,
		.priority	= NF_IP_PRI_IP_DEFEND_FLOOD_SRC,
	},
};

#if 0	//delete by yuchao
void ip_defend_interface_flood_age(unsigned long data)
{
	flood_defend_item_t *src_item, *src_next_item;
	flood_defend_item_t *item, *next_item;
	int i;

	for (i = 0; i < g_flood_hash_size; i++) {
		if (g_ip_defend_flood.flood_src_hash) {
			spin_lock_bh(&(g_ip_defend_flood.flood_src_hash[i].lock));
			list_for_each_entry_safe_rcu(src_item, src_next_item, &(g_ip_defend_flood.flood_src_hash[i].head), list) {
				if (src_item->attack_timer) {
					if (time_after_eq(jiffies, src_item->attack_timer + anti_flood_block_time * HZ)) {

						src_item->attack_timer = 0;
						printk("block end, srcip %pI4 attack_timer reset\n", &src_item->ip);
						//continue;
					}
				}
				if (time_after_eq(jiffies, src_item->match_timer + anti_flood_item_timout * HZ)) {

					list_del_rcu(&(src_item->list));
					atomic_dec(&g_interface_flood_item_src_num);
					call_rcu(&(src_item->rcu), interface_flood_free);
				}
			}
			spin_unlock_bh(&(g_ip_defend_flood.flood_src_hash[i].lock));
		}

		if (g_ip_defend_flood.flood_dst_hash) {
			spin_lock_bh(&(g_ip_defend_flood.flood_dst_hash[i].lock));
			list_for_each_entry_safe_rcu(item, next_item, &(g_ip_defend_flood.flood_dst_hash[i].head), list) {
				if (time_after_eq(jiffies, item->match_timer + 30 * HZ)) {

					list_del_rcu(&(item->list));
					atomic_dec(&g_interface_flood_item_dst_num);
					call_rcu(&(item->rcu), interface_flood_free);
				}
			}
			spin_unlock_bh(&(g_ip_defend_flood.flood_dst_hash[i].lock));
		}
	}

	mod_timer(&g_ip_defend_flood.timeout, jiffies + (60 * HZ));
}

static int ip_defend_interface_flood_info_init(void)
{
	init_timer(&(g_ip_defend_flood.timeout));
	g_ip_defend_flood.timeout.function = ip_defend_interface_flood_age;
	g_ip_defend_flood.timeout.expires = jiffies + (60 * HZ);
	add_timer(&(g_ip_defend_flood.timeout));

	return 0;
}

static int ip_defend_interface_dst_flood_init(void)
{
	int i;
	g_ip_defend_flood.flood_dst_hash = kmalloc(g_flood_hash_size * sizeof(struct ip_defend_interface_flood_head), GFP_KERNEL);
	if (!g_ip_defend_flood.flood_dst_hash) {
		return -1;
	}

	for (i = 0; i < g_flood_hash_size; i++) {
		INIT_LIST_HEAD(&(g_ip_defend_flood.flood_dst_hash[i].head));
		spin_lock_init(&(g_ip_defend_flood.flood_dst_hash[i].lock));
	}

	return 0;
}
#endif

static int ip_defend_interface_src_flood_init(void)
{
	int i;

	g_ip_defend_flood.flood_src_hash = kmalloc(g_flood_hash_size * sizeof(struct ip_defend_interface_flood_head), GFP_KERNEL);
	if (!g_ip_defend_flood.flood_src_hash) {
		return -1;
	}

	for (i = 0; i < g_flood_hash_size; i++) {
		INIT_LIST_HEAD(&(g_ip_defend_flood.flood_src_hash[i].head));
		spin_lock_init(&(g_ip_defend_flood.flood_src_hash[i].lock));
	}
	//printk("enter ip_defend_interface_src_flood_init\n");

	return 0;
}

static int __init ip_defend_flood_init(void)
{
	//int i;

	//printk("enter ip_defend_flood_init\n");
	g_flood_hash_size = 8 * 1024;
	g_interface_flood_item_max = 100000;

/*	g_icmpfld_hash = kmalloc(g_flood_hash_size * sizeof(struct ip_defend_flood_head), GFP_KERNEL);
	if (g_icmpfld_hash == NULL) {
		return 1;
	}

	g_udpfld_hash = kmalloc(g_flood_hash_size * sizeof(struct ip_defend_flood_head), GFP_KERNEL);
	if (g_udpfld_hash == NULL) {
		kfree(g_icmpfld_hash);
		return 1;
	}

	g_synfld_hash = kmalloc(g_flood_hash_size * sizeof(struct ip_defend_flood_head), GFP_KERNEL);
	if (g_synfld_hash == NULL) {
		kfree(g_icmpfld_hash);
		kfree(g_udpfld_hash);
		return 1;
	}*/

	/* 初始化Hash数组的各表头 */
	/*for (i = 0; i < g_flood_hash_size; i++) {
		INIT_LIST_HEAD(&(g_icmpfld_hash[i].head));
		g_icmpfld_hash[i].count = 0;
		INIT_LIST_HEAD(&(g_udpfld_hash[i].head));
		g_udpfld_hash[i].count = 0;
		INIT_LIST_HEAD(&(g_synfld_hash[i].head));
		g_synfld_hash[i].count = 0;
	}*/

	ip_defend_interface_src_flood_init();
#if 0	//delete by yuchao
	ip_defend_interface_dst_flood_init();
	ip_defend_interface_flood_info_init();
#endif

	//创建proc文件，用户态接口
	anti_flood_proc_init();

	//if (nf_register_hooks(ip_defend_flood_dst_ops, ARRAY_SIZE(ip_defend_flood_dst_ops)) < 0) {
	//	printk("can't register ip defend flood hooks.\n");
	//	return 1;
	//}

	if (nf_register_hooks(ip_defend_flood_src_ops, ARRAY_SIZE(ip_defend_flood_src_ops)) < 0) {
		printk("can't register ip defend flood hooks.\n");
		return 1;
	}

	return 0;
}

static void __exit ip_defend_flood_exit(void)
{
	//删除钩子函数
	//nf_unregister_hooks(ip_defend_flood_dst_ops, ARRAY_SIZE(ip_defend_flood_dst_ops));
	nf_unregister_hooks(ip_defend_flood_src_ops, ARRAY_SIZE(ip_defend_flood_src_ops));

	//删除proc文件
	anti_flood_proc_exit();

	//释放hash 表内存
	ip_defend_interface_flood_kfree();
}

module_init(ip_defend_flood_init);
module_exit(ip_defend_flood_exit);

MODULE_LICENSE("GPL");

