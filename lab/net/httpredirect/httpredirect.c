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
#include <net/tcp.h>

#include <linux/proc_fs.h>
#include <linux/fs.h>


//#define HTTP_REDIRECT_DEBUG

#define HTTP_302_HEADER "HTTP/1.1 302 Found\r\n\
Location: %s\r\n\
Content-Length: 0\r\n\r\n"
#define URL_SIZE 128
static char http_404redirect_url[URL_SIZE] = {0};
static int http_404redirect_enable = 0;
static unsigned int http404_nf_cb(void *priv,
					struct sk_buff *skb,
					const struct nf_hook_state *state)
{
	struct iphdr *iph = NULL;
	struct tcphdr *tcph = NULL;
	sk_buff_data_t tcp_payload = NULL;
	char http_header[20] = {0};
	char *http_404_pos = NULL;
	int count = 0;
#ifdef HTTP_REDIRECT_DEBUG
	struct sk_buff *skb2;
#endif
	if(!skb)
		return NF_DROP;
	iph = ip_hdr(skb);
	
	if(iph->protocol == IPPROTO_TCP && http_404redirect_enable){
		tcph = tcp_hdr(skb);
		tcp_payload = (sk_buff_data_t)(tcph) + tcp_hdrlen(skb);
	
#ifdef HTTP_REDIRECT_DEBUG
		print_hex_dump(KERN_ERR,"http header:", DUMP_PREFIX_OFFSET, 16, 1, tcp_payload, 50, true);
#endif
		strncpy(http_header, tcp_payload, sizeof(http_header) - 1);
#ifdef HTTP_REDIRECT_DEBUG

		printk("========> original http_header: %s", http_header);
#endif
		http_404_pos = strnstr(tcp_payload + 8, "404", 10);
		if(http_404_pos && strnstr(tcp_payload, "HTTP", 5)){
			sprintf(tcp_payload, HTTP_302_HEADER, http_404redirect_url);
#ifdef HTTP_REDIRECT_DEBUG
		printk("========> changed http_header: %s", tcp_payload);
#endif
			count = skb->tail - tcp_payload + strlen(tcp_payload) + 1;
			memset(tcp_payload + strlen(tcp_payload) + 1, 0x00, count);
			tcph->check = 0;
			if (skb->ip_summed == CHECKSUM_PARTIAL) {
				tcph->check = ~tcp_v4_check(ntohs(iph->tot_len) - ip_hdrlen(skb), iph->saddr, iph->daddr, 0);
				skb->csum_start = skb_transport_header(skb) - skb->head;
				skb->csum_offset = offsetof(struct tcphdr, check);
			}
			else{
				tcph->check = tcp_v4_check(skb_tail_pointer(skb) - skb_transport_header(skb), iph->saddr, iph->daddr,
									 csum_partial(tcph, skb_tail_pointer(skb) - skb_transport_header(skb), 0));
			}
			
			iph->check = 0;
			iph->check = ip_compute_csum(iph, skb_transport_header(skb) - skb_network_header(skb));
#ifdef HTTP_REDIRECT_DEBUG
			skb2 = skb_clone(skb, GFP_ATOMIC);
			skb2->protocol = __constant_htons(ETH_P_802_3);
			skb_reset_network_header(skb2);
			skb_reset_mac_header(skb2);
			skb_push(skb2, sizeof(struct ethhdr));
			skb2->dev = __dev_get_by_name(&init_net, "enp2s0");
			if(dev_queue_xmit(skb2)){
				kfree_skb(skb2);
			}
#endif
			
		}
	}

	return NF_ACCEPT;
}




static struct nf_hook_ops test_nf_ops[] = {
	{
		.hook = http404_nf_cb,
		.pf = PF_INET,
		.hooknum = NF_INET_LOCAL_OUT,
		.priority = NF_IP_PRI_FILTER,
	},
	{
		.hook = http404_nf_cb,
		.pf = PF_INET,
		.hooknum = NF_INET_FORWARD,
		.priority = NF_IP_PRI_FIRST,
	},
};




static int test_conntrack_fs_open(struct inode *inode, struct file *fd)
{
	return 0;
}

static ssize_t test_conntrack_fs_read(struct file *fd, char __user *buf, size_t size, loff_t *lof)
{
	printk("enable: %d url: %s\n", http_404redirect_enable, http_404redirect_url);
	return 0;
}

static ssize_t test_conntrack_fs_write(struct file *fd, const char __user *buf, size_t size, loff_t *lof)
{
	if(URL_SIZE < size)
		goto out;
	sscanf(buf, "%d %s", &http_404redirect_enable, http_404redirect_url);
	
out:
    return size;
}

static int test_conntrack_fs_release(struct inode *inode, struct file *fd)
{
	return 0;
}

//call this func in module_init func
static int __net_init test_conntrack_init(struct net *net)
{

	return nf_register_hooks(test_nf_ops, ARRAY_SIZE(test_nf_ops));
}
//call this func in module_exit func
static void __net_exit  test_conntrack_exit(struct net *net)
{
	nf_unregister_hooks(test_nf_ops, ARRAY_SIZE(test_nf_ops));

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
    proc_create("http_404redirect", S_IRWXU|S_IRWXG|S_IRWXO, init_net.proc_net, &test_conntrack_ops);
	return register_pernet_subsys(&test_conntrack_net_ops);	
}

module_init(test_module_init);

static void __exit test_module_exit(void)
{
	remove_proc_entry("http_404redirect", init_net.proc_net);
	unregister_pernet_subsys(&test_conntrack_net_ops);
}

module_exit(test_module_exit);

MODULE_LICENSE("GPL v2");


