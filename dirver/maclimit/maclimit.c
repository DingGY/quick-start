/***********************************************************************
 *  name: nf_maclimit, module of build in
 *  Author: dingguangyu
 *  date: 2018.9.8
 *
 * HGUB963XX-264 dingguangyu add for user number limit for internet 2018.9.8
************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/if_ether.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/netfilter_arp.h>
#include <linux/kthread.h>
#include <linux/if_arp.h>
#include <net/arp.h>

//#define DEBUG
#ifdef DEBUG
	#define MACLIMIT_DEBUG(fmt, arg...) \
		printk("maclimit filter: "); \
		printk(fmt, ##arg)
#else
	#define MACLIMIT_DEBUG(fmt, arg...)
#endif


#define PROC_RECEVE_BUF_SIZE 20
#define PROC_SEND_BUF_SIZE 255
#define CHECK_ARP_WAIT_TIME_SEC 0.2
#define CHECK_ARP_PACKET_NUM 5

#define BOARDCAST_ADDRESS {0Xff,0xff,0xff,0xff,0xff,0xff}

static DECLARE_WAIT_QUEUE_HEAD(check_task_wait_queue);


#define NMACTOLSTR(addr) \
	((unsigned char *)addr)[0], \
	((unsigned char *)addr)[1], \
	((unsigned char *)addr)[2], \
	((unsigned char *)addr)[3], \
	((unsigned char *)addr)[4], \
	((unsigned char *)addr)[5]

#define NIPTOLSTR(addr) \
	((unsigned char *)&addr)[0], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[3]

#pragma pack(push,1)
struct arp_info {
    unsigned char src[ETH_ALEN];
    __be32 srcip;
    unsigned char dst[ETH_ALEN];
    __be32 dstip;    
};
#pragma pack(pop)

typedef struct maclimit_cfg_s
{
	int enable;
	long limit_num;
} maclimit_cfg_t;

typedef struct maclimit_node_s
{		
	struct net_device *dev;
	unsigned char mac[ETH_ALEN];
    __be32 srcip;
	__be32 dstip;
	struct list_head list;
} maclimit_node_t;


enum add_mode {
	NO_ADD_TASK,
	ADD_IMMEDIATE,
	ADD_CHECK,
	ARP_REPLAY_CHECK,
	ARP_REPLAY_OK,
};


typedef struct new_terminal_s
{
	maclimit_node_t node;
	unsigned char arp_cmp_mac[ETH_ALEN];
	atomic_t flag; 
} new_terminal_t;

static struct task_struct *limit_check_task;

static LIST_HEAD(maclimit_list);


static new_terminal_t new_terminal;
static rwlock_t maclimit_rwlock;


static maclimit_cfg_t maclimit_cfg = {
	.enable = 0,
	.limit_num = 1,
};


static maclimit_node_t * create_new_node(maclimit_node_t new)
{
	maclimit_node_t *maclimit = (maclimit_node_t *)kmalloc(sizeof(maclimit_node_t), GFP_KERNEL);
	if(maclimit == NULL)
	{
		MACLIMIT_DEBUG("can not malloc a maclimit node\n");
		return NULL;
	}
	maclimit->srcip = new.srcip;
	maclimit->dstip = new.dstip;
	maclimit->dev = new.dev;
	memcpy(maclimit->mac, new.mac, ETH_ALEN);
	printk("add new limit terminal srcmac %02x:%02x:%02x:%02x:%02x:%02x\r\n", NMACTOLSTR(maclimit->mac));
	return maclimit;
}

static int limit_check_func(void *data)
{
	maclimit_node_t *temp, *pos;
	maclimit_node_t *node = NULL;
	int i;
	const unsigned char boardcast_address[] = BOARDCAST_ADDRESS;
	while(!kthread_should_stop())
	{
		MACLIMIT_DEBUG("DingGY in kernel thread\n");
		switch(atomic_read(&new_terminal.flag))
		{
			case ADD_IMMEDIATE:
				MACLIMIT_DEBUG("in ADD_IMMEDIATE state\n");
				if((node = create_new_node(new_terminal.node)) == NULL)
					break;
				write_lock(&maclimit_rwlock);
				list_add(&node->list, &maclimit_list);
				write_unlock(&maclimit_rwlock);
				break;
			case ADD_CHECK:
				MACLIMIT_DEBUG("in ADD_CHECK state\n");
				list_for_each_entry_safe(pos, temp, &maclimit_list, list) 
				{		
					atomic_set(&new_terminal.flag, ARP_REPLAY_CHECK);	
					memcpy(new_terminal.arp_cmp_mac, pos->mac, ETH_ALEN);
					for(i  = 0; 
						(i < CHECK_ARP_PACKET_NUM) && (atomic_read(&new_terminal.flag) == ARP_REPLAY_CHECK); 
						i++)
					{
						arp_send(ARPOP_REQUEST, ETH_P_ARP,
							pos->srcip, pos->dev,
							pos->dstip, NULL,
							pos->dev->dev_addr, 
							boardcast_address);
						MACLIMIT_DEBUG("send arp state\n");
						set_current_state(TASK_UNINTERRUPTIBLE); 
						schedule_timeout(CHECK_ARP_WAIT_TIME_SEC * HZ);
					}

					/* not be change by arp */
					if(atomic_read(&new_terminal.flag) == ARP_REPLAY_CHECK)
					{
						/* find a place whitch can be replaced by new terminal */
						MACLIMIT_DEBUG("recved arp\n");
						if((node = create_new_node(new_terminal.node)) == NULL)
							break;
						write_lock(&maclimit_rwlock);
						MACLIMIT_DEBUG("get write lock\n");
						create_new_node(new_terminal.node);
						list_replace(&pos->list, &node->list);
						kfree(pos);
						write_unlock(&maclimit_rwlock);
						MACLIMIT_DEBUG("find a place to insert\n");
						break;
					}
				}
				break;
			case NO_ADD_TASK:
			case ARP_REPLAY_CHECK:
			default:
				break;
		}
		atomic_set(&new_terminal.flag, NO_ADD_TASK);
		set_current_state(TASK_UNINTERRUPTIBLE); 
		schedule();
	}
	return 0;
}


static void send_add_opt_to_thread(new_terminal_t *terminal,const __be32 srcip, const __be32 dstip, const unsigned char *srcmac, struct net_device *dev, int flag)
{
	if(atomic_read(&terminal->flag) == NO_ADD_TASK)
	{
		terminal->node.srcip = srcip;
		terminal->node.dstip = dstip;
		terminal->node.dev = dev;
		memcpy(terminal->node.mac, srcmac, ETH_ALEN);
		atomic_set(&terminal->flag, flag);
		wake_up_process(limit_check_task);
		MACLIMIT_DEBUG("send add opt msg successed %d\n", atomic_read(&terminal->flag));
	}
	MACLIMIT_DEBUG("send add opt msg failed %d\n", atomic_read(&terminal->flag));
}
static unsigned int net_hook_func(
		const struct nf_hook_ops *ops,
		//void *prev,
		struct sk_buff *skb,
		const struct nf_hook_state *state)
{
	__be32 srcip,dstip;
	char srcmac[ETH_ALEN], dstmac[ETH_ALEN];
	struct iphdr *iph = NULL;
	struct ethhdr *mach = NULL;
	maclimit_node_t *pos;
	int connect_num = 0;
	if(skb && maclimit_cfg.enable)
	{
		iph = ip_hdr(skb);
		mach = eth_hdr(skb);
		//if(iph->protocol == IPPROTO_ICMP && !strcmp(state->in->name,"br0"))
		if(iph->protocol != IPPROTO_UDP && !strcmp(state->in->name,"br0"))
		{
			memcpy(srcmac, mach->h_source, ETH_ALEN);
			memcpy(dstmac, mach->h_dest, ETH_ALEN);
			srcip = iph->saddr;
			dstip = iph->daddr;
			// MACLIMIT_DEBUG(
			// 	"-----------------------LIMIT packet: -----------------------\r\n"
			// 	"srcmac %02x:%02x:%02x:%02x:%02x:%02x\r\n"
			// 	"dstmac %02x:%02x:%02x:%02x:%02x:%02x\r\n"
			// 	"sip : %d.%d.%d.%d \r\n"
			// 	"dip : %d.%d.%d.%d \r\n"
			// 	"dev in:%s, dev out:%s\r\n",
			// 	NMACTOLSTR(srcmac), 
			// 	NMACTOLSTR(dstmac), 
			// 	NIPTOLSTR(srcip),
			// 	NIPTOLSTR(dstip),
			// 	state->in->name, state->out->name);
			
			if(!read_trylock(&maclimit_rwlock))
				return NF_ACCEPT;
			list_for_each_entry(pos, &maclimit_list, list)
			{
				/* searce the maclimit list for packet */
				connect_num++;
				
				if(memcmp(pos->mac, srcmac, ETH_ALEN) == 0)
				{
					/* if fonuded the same mac address in the list then let it go */
					read_unlock(&maclimit_rwlock);
					return NF_ACCEPT;
				}
			}
			read_unlock(&maclimit_rwlock);

			if(connect_num < maclimit_cfg.limit_num)	
			{
				/* if the list is not full, then add the mac address to the list 
				 *	ohterwhise drop it 
				 */
				send_add_opt_to_thread(&new_terminal, srcip, dstip, srcmac, state->in, ADD_IMMEDIATE);
				return NF_ACCEPT;
			}
			else
			{
				send_add_opt_to_thread(&new_terminal, srcip, dstip, srcmac,state->in, ADD_CHECK);
				return NF_DROP;
			}
		}
	}
	return NF_ACCEPT;
}



static unsigned int arpin_hook_func(
		const struct nf_hook_ops *ops,
		//void *prev,
		struct sk_buff *skb,
		const struct nf_hook_state *state)
{
	struct arphdr *arph = NULL;
    struct arp_info *arpinfo = NULL;
	if(skb)
	{
		arph = arp_hdr(skb);
		if(arph == NULL)
		{
			MACLIMIT_DEBUG("null arp header\n");
			return NF_ACCEPT;
		}
		arpinfo = (struct arp_info *)(arph + 1);
		MACLIMIT_DEBUG(
			"\n-------------  ARP IN --------------\r\n"
			"smac : %02x:%02x:%02x:%02x:%02x:%02x \r\n"
			"sip : %d.%d.%d.%d \r\n"
			"dmac : %02x:%02x:%02x:%02x:%02x:%02x \r\n"
			"dip : %d.%d.%d.%d \r\n"
			"----------------  END  --------------\r\n",
			NMACTOLSTR(arpinfo->src),
			NIPTOLSTR(arpinfo->srcip),
			NMACTOLSTR(arpinfo->dst),
			NIPTOLSTR(arpinfo->dstip)
		);
		if(atomic_read(&new_terminal.flag) == ARP_REPLAY_CHECK)
		{
			if(memcmp(arpinfo->src, new_terminal.arp_cmp_mac, ETH_ALEN) == 0)
			{
				atomic_set(&new_terminal.flag, ARP_REPLAY_OK);
				wake_up_process(limit_check_task);
			}
		}
	}
	return NF_ACCEPT;
}

#ifdef DEBUG
static unsigned int arpout_hook_func(
		const struct nf_hook_ops *ops,
		//void *prev,
		struct sk_buff *skb,
		const struct nf_hook_state *state)
{
	struct arphdr *arph = NULL;
    struct arp_info *arpinfo = NULL;
	if(skb)
	{
		arph = arp_hdr(skb);
		if(arph == NULL)
		{
			MACLIMIT_DEBUG("null arp header\n");
			return NF_ACCEPT;
		}
		arpinfo = (struct arp_info *)(arph + 1);
		MACLIMIT_DEBUG(
			"\n-------------  ARP OUT --------------\r\n"
			"smac : %02x:%02x:%02x:%02x:%02x:%02x \r\n"
			"sip : %d.%d.%d.%d \r\n"
			"dmac : %02x:%02x:%02x:%02x:%02x:%02x \r\n"
			"dip : %d.%d.%d.%d \r\n"
			"----------------  END  ---------------\r\n",
			NMACTOLSTR(arpinfo->src),
			NIPTOLSTR(arpinfo->srcip),
			NMACTOLSTR(arpinfo->dst),
			NIPTOLSTR(arpinfo->dstip)
		);
	}
	return NF_ACCEPT;
}
#endif

static struct nf_hook_ops nfhos[] = {
	{
		.list = {NULL,NULL},
		.hook = net_hook_func,
		.hooknum =NF_INET_PRE_ROUTING,
		.pf = PF_INET,
		.priority = NF_IP_PRI_FIRST + 2,
	},
	{
		.list = {NULL,NULL},
		.hook = arpin_hook_func,
		.hooknum = NF_ARP_IN,
		.pf = NFPROTO_ARP,
		.priority = NF_IP_PRI_FIRST + 1,
	},
#ifdef DEBUG
	{
		.list = {NULL,NULL},
		.hook = arpout_hook_func,
		.hooknum = NF_INET_FORWARD,
		.pf = NFPROTO_ARP,
		.priority = NF_IP_PRI_FIRST + 1,
	},
#endif
};




static ssize_t read_maclimit_proc(struct file *f, char *buf, size_t cnt, loff_t *pos)
{
	char page[PROC_SEND_BUF_SIZE];
	size_t len;

	len = sprintf(
		page, 
		"config: limit_num: %ld, enable: %d\n", 
		maclimit_cfg.limit_num, 
		maclimit_cfg.enable
		);

	return simple_read_from_buffer(buf, cnt, pos, page, len);
}




static ssize_t write_maclimit_proc(struct file *f, const char *buffer, size_t count, loff_t *pos)
{
	char *opt;
	char buf[PROC_RECEVE_BUF_SIZE];
	int opt_num = 0,i = 0;
	if(count > PROC_RECEVE_BUF_SIZE)
	{
		return -EFAULT;
	}
	if(copy_from_user(buf, buffer, count)) 
	{
		return -EFAULT;
	}

	for(i = 0; i < count; i++)
	{
		if((buf[i] == ' ' && buf[i+1] != '\0' && buf[i+1] != ' ') || i == 0)
		{
			
			switch(opt_num)
			{
				case 0:
					opt = &buf[i];
					maclimit_cfg.limit_num = simple_strtoul(opt , NULL, 0);
					break;
				case 1:
					opt = &buf[i + 1];
					maclimit_cfg.enable = simple_strtoul(opt , NULL, 0);
					break;
				default:
					break;
			}
			opt_num++;
		}
	}
	printk(
		"maclimit config success!\n limit_num: %ld, enable: %d\n", 
		maclimit_cfg.limit_num, 
		maclimit_cfg.enable
		);
	return count;
}




static struct file_operations nf_maclimit_file_ops = {
	.owner		= THIS_MODULE,
	.read		= read_maclimit_proc,
	.write		= write_maclimit_proc,
};



static int __init nf_init(void)
{
	int status = 0;
	if (!proc_create("maclimit", S_IRWXUGO,
			 init_net.nf.proc_netfilter, &nf_maclimit_file_ops))
	{
		status = -EFAULT;
		pr_err("failed to register pernet ops\n");
		goto cleanup_proc;
	}

	status = nf_register_hooks(nfhos,ARRAY_SIZE(nfhos));
	if (status < 0) {
		pr_err("failed to register nf hook\n");
		goto cleanup_nf_hook;
	}
	limit_check_task = kthread_create(limit_check_func,NULL,"maclimit");
	if (IS_ERR(limit_check_task)){
		pr_err("failed to start maclimit kernel thread\n");
		goto cleanup_nf_hook;
	}
	rwlock_init(&maclimit_rwlock);
	wake_up_process(limit_check_task);
	return status;
cleanup_nf_hook:
	nf_unregister_hooks(nfhos,ARRAY_SIZE(nfhos));
cleanup_proc:	
	remove_proc_entry("maclimit", init_net.nf.proc_netfilter);
	return status;
}


static void __exit nf_cleanup(void)
{
	maclimit_node_t *temp, *pos;
	if(!IS_ERR(limit_check_task)){
		printk("stop kernel thread\n");
		kthread_stop(limit_check_task);
		limit_check_task = NULL;
	}
	remove_proc_entry("maclimit", init_net.nf.proc_netfilter);
	nf_unregister_hooks(nfhos,ARRAY_SIZE(nfhos));
	list_for_each_entry_safe(pos, temp, &maclimit_list, list) 
	{
		kfree(pos);
	}
}

module_init(nf_init);
module_exit(nf_cleanup);
MODULE_AUTHOR("DingGY");
MODULE_LICENSE("GPL");
