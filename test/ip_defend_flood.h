#ifndef _IP_DEFEND_FLOOD_H
#define _IP_DEFEND_FLOOD_H

typedef unsigned int be32;

struct ip_defend_flood {
	be32       start_ip;
	be32	   end_ip;
	u_int8_t  synflood;
	u_int8_t  udpflood;
	u_int8_t  icmpflood;
	u_int8_t  src_synflood;
	u_int8_t  src_udpflood;
	u_int8_t  src_icmpflood;
	u_int32_t  synflood_threshold;
	u_int32_t  udpflood_threshold;
	u_int32_t  icmpflood_threshold;
	u_int32_t  src_synflood_threshold;
	u_int32_t  src_udpflood_threshold;
	u_int32_t  src_icmpflood_threshold;

	int		   is_add; /* add or edit */
	char       ifname[IFNAMSIZ];
};

#define IP_DEFEND_DST_SYNFLOOD        0x01
#define IP_DEFEND_DST_UDPFLOOD        0x02
#define IP_DEFEND_DST_ICMPFLOOD       0x04
#define IP_DEFEND_SRC_SYNFLOOD        0x08
#define IP_DEFEND_SRC_UDPFLOOD        0x10
#define IP_DEFEND_SRC_ICMPFLOOD       0x20

enum ip_defend_cmd {
	IP_DEFEND_CFG_SYNFLOOD_SET,
	IP_DEFEND_CFG_UDPFLOOD_SET,
	IP_DEFEND_CFG_ICMPFLOOD_SET,
	IP_DEFEND_CFG_FLOOD_ADD,
	IP_DEFEND_CFG_FLOOD_MOD,
	IP_DEFEND_CFG_FLOOD_DEL,
	IP_DEFEND_FLOOD_ATTACK_LOG,
	IP_DEFEND_FLOOD_ATTACK_LOG_GET,
};

enum ip_attack_type {
	synflood,
	udpflood,
	icmpflood,
};

struct log_attack {
	char sip[42];
	char dip[42];
	int sport;
	int dport;
	char protocol[32];
	char smac[32];
	int count;
	char inifname[32];
	char stime[32];
	char etime[32];
};

struct flood_tbf {
	unsigned long attack_start_time;
	unsigned long attack_end_time;
	unsigned long  prev_jiff;
	spinlock_t tbf_lock;          /* 锁变量 */
	u32 curr_token;         /* 当前令牌数 */
	u32 threshold;          /* 阈值 */
	u32 attack_count;
	u8 attack_state;
};
enum flood_token {
	FLOOD_GET_TOKEN,        /* 得到令牌标志 */
	FLOOD_GET_NO_TOKEN          /* 得不到令牌标志 */
};

typedef struct flood_item {
	be32   ip;                /* 目标IP地址 */
	struct flood_tbf tbf;      /* 速率计算的令牌桶结构 */
} flood_item_t;

typedef struct flood_tbl_item {
	struct list_head hash;
	struct rcu_head  rcu;
	flood_item_t item;
} flood_tbl_item_t;

struct flood_defend {
	struct flood_tbf tbf;
};

typedef struct flood_defend_item {
	struct list_head list;
	struct rcu_head  rcu;
	be32 ip;
	unsigned long  match_timer; //jiffies
	unsigned long  attack_timer;//jiffies
	struct flood_defend  synflood;
	struct flood_defend  udpflood;
	struct flood_defend  icmpflood;
	struct flood_defend  dnsflood;
	u16  defend_flag;
} flood_defend_item_t;

struct ip_defend_flood_cfg {
	struct list_head list;
	be32       start_ip;
	be32       end_ip;
	u_int32_t  synflood_threshold;
	u_int32_t  udpflood_threshold;
	u_int32_t  icmpflood_threshold;
	u_int16_t  synflood;
	u_int16_t  udpflood;
	u_int16_t  icmpflood;
};

struct ip_defend_interface_flood_cfg {
	u_int32_t  dst_synflood_threshold;
	u_int32_t  dst_udpflood_threshold;
	u_int32_t  dst_icmpflood_threshold;
	u_int32_t  src_synflood_threshold;
	u_int32_t  src_udpflood_threshold;
	u_int32_t  src_icmpflood_threshold;
	u_int16_t  defend_flag;
};

struct ip_defend_flood_head {
	struct list_head head;
	int count;
};

struct ip_defend_interface_flood_head {
	struct list_head head;
	spinlock_t lock;
};

struct ip_defend_flood_info {
	struct ip_defend_interface_flood_head * flood_src_hash;
	struct ip_defend_interface_flood_head * flood_dst_hash;
	struct ip_defend_interface_flood_cfg defend_cfg;
	struct timer_list timeout;
};

static inline void get_str_from_macaddr(unsigned char * mac, char * str)
{
	sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", mac[ 0 ], mac[ 1 ], mac[ 2 ], mac[ 3 ], mac[ 4 ], mac[ 5 ]);
	return ;
}

void ip_defend_attack_info_out(int attack_type, const struct sk_buff *skb, unsigned long time_start, unsigned long time_end, unsigned int attack_count);

#endif


