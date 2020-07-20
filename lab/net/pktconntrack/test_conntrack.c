#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ip.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_arp.h>
#include <linux/in_route.h>
#include <net/ip.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
static __u32 cmp_src_ip = 0;

static unsigned int test_nf_pre_routing(void *priv,
					struct sk_buff *skb,
					const struct nf_hook_state *state)
{
	struct iphdr* iph = NULL;
	if(!skb)
		return NF_ACCEPT;
	iph = ip_hdr(skb);
	
	if(ntohl(iph->saddr) == cmp_src_ip)
		printk("<================ <%s:%d> ================>\n", __FUNCTION__, __LINE__);

	return NF_ACCEPT;
}

static unsigned int test_nf_local_in(void *priv,
					struct sk_buff *skb,
					const struct nf_hook_state *state)
{
	struct iphdr* iph = NULL;
	if(!skb)
		return NF_ACCEPT;
	iph = ip_hdr(skb);
	
	if(ntohl(iph->saddr) == cmp_src_ip)
		printk("<================ <%s:%d> ================>\n", __FUNCTION__, __LINE__);
	return NF_ACCEPT;
}

static unsigned int test_nf_forward(void *priv,
					struct sk_buff *skb,
					const struct nf_hook_state *state)

{
	struct iphdr* iph = NULL;
	if(!skb)
		return NF_ACCEPT;
	iph = ip_hdr(skb);
	
	if(ntohl(iph->saddr) == cmp_src_ip)
		printk("<================ <%s:%d> ================>\n", __FUNCTION__, __LINE__);
	return NF_ACCEPT;
}

static unsigned int test_nf_local_out(void *priv,
					struct sk_buff *skb,
					const struct nf_hook_state *state)

{
	struct iphdr* iph = NULL;
	if(!skb)
		return NF_ACCEPT;
	iph = ip_hdr(skb);
	
	if(ntohl(iph->saddr) == cmp_src_ip)
		printk("<================ <%s:%d> ================>\n", __FUNCTION__, __LINE__);
	return NF_ACCEPT;
}

static unsigned int test_nf_post_routing(void *priv,
					struct sk_buff *skb,
					const struct nf_hook_state *state)
{
	struct iphdr* iph = NULL;
	if(!skb)
		return NF_ACCEPT;
	iph = ip_hdr(skb);
	
	if(ntohl(iph->saddr) == cmp_src_ip)
		printk("<================ <%s:%d> ================>\n", __FUNCTION__, __LINE__);
	return NF_ACCEPT;
}


static struct nf_hook_ops test_nf_ops[] = {
	{
		.hook = test_nf_pre_routing,
		.pf = PF_INET,
		.hooknum = NF_INET_PRE_ROUTING,
		.priority = NF_IP_PRI_FILTER,
	},
	{
		.hook = test_nf_local_in,
		.pf = PF_INET,
		.hooknum = NF_INET_LOCAL_IN,
		.priority = NF_IP_PRI_FILTER,
	},
	{
		.hook = test_nf_forward,
		.pf = PF_INET,
		.hooknum = NF_INET_FORWARD,
		.priority = NF_IP_PRI_FILTER,
	},

	{
		.hook = test_nf_local_out,
		.pf = PF_INET,
		.hooknum = NF_INET_LOCAL_OUT,
		.priority = NF_IP_PRI_FILTER,
	},

	{
		.hook = test_nf_post_routing,
		.pf = PF_INET,
		.hooknum = NF_INET_POST_ROUTING,
		.priority = NF_IP_PRI_FILTER,
	},

};


unsigned int ip_str_to_num(const char *buf)
 
{
    unsigned int tmpip[4] = {0};
    unsigned int tmpip32 = 0;
    sscanf(buf, "%d.%d.%d.%d", &tmpip[3], &tmpip[2], &tmpip[1], &tmpip[0]);
    tmpip32 = (tmpip[3]<<24) | (tmpip[2]<<16) | (tmpip[1]<<8) | tmpip[0];
    return tmpip32;
}

static int test_conntrack_fs_open(struct inode *inode, struct file *fd)
{
	return 0;
}

static ssize_t test_conntrack_fs_read(struct file *fd, char __user *buf, size_t size, loff_t *lof)
{
	return 0;
}

static ssize_t test_conntrack_fs_write(struct file *fd, const char __user *buf, size_t size, loff_t *lof)
{
	cmp_src_ip = ip_str_to_num(buf);
	printk("<%s:%d> 0x%04x\n", __FUNCTION__, __LINE__, cmp_src_ip);
    return size;
}

static int test_conntrack_fs_release(struct inode *inode, struct file *fd)
{
	return 0;
}

//call this func in module_init func
static int __net_init test_conntrack_init(struct net *net)
{

	return nf_register_net_hooks(net, test_nf_ops,
				   ARRAY_SIZE(test_nf_ops));
}
//call this func in module_exit func
static void __net_exit  test_conntrack_exit(struct net *net)
{
	nf_unregister_net_hooks(net, test_nf_ops,
			  ARRAY_SIZE(test_nf_ops));

}
static struct pernet_operations test_conntrack_net_ops = {
	.init = test_conntrack_init,
	.exit = test_conntrack_exit,
};



static const struct file_operations test_conntrack_ops = {
	.owner = THIS_MODULE,
	.open = test_conntrack_fs_open,
	.read = test_conntrack_fs_read,
	.write = test_conntrack_fs_write,
	.release = test_conntrack_fs_release,
};
static int __init test_module_init(void)
{
	printk("<================ <%s:%d> ================>\n", __FUNCTION__, __LINE__);
    proc_create("test_conntrack", S_IRWXU|S_IRWXG|S_IRWXO, init_net.proc_net, &test_conntrack_ops);
	return register_pernet_subsys(&test_conntrack_net_ops);	
}

module_init(test_module_init);

static void __exit test_module_exit(void)
{
	printk("<================ <%s:%d> ================>\n", __FUNCTION__, __LINE__);
	remove_proc_entry("test_conntrack", init_net.proc_net);
	unregister_pernet_subsys(&test_conntrack_net_ops);
}

module_exit(test_module_exit);

MODULE_LICENSE("GPL v2");

