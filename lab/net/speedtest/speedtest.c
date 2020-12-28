#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <net/net_namespace.h>
#include <linux/netdevice.h>
#include <net/ip.h>
#include <linux/udp.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/kfifo.h>

#define ST_DEBUG
#ifdef ST_DEBUG
#define ST_DBG(str, args...)	\
	printk("[speedtest]::<%s:%d> "str, __FUNCTION__, __LINE__ , ##args)
#else
#define ST_DBG(str, args...)
#endif

#define ST_ERR(str, args...)	\
	printk("[speedtest err]::<%s:%d> "str, __FUNCTION__, __LINE__, ##args)



#define SPEEDTEST_LOOPDETECT_TIMER_CIRCLE 2 //sceond

#define SPEEDTEST_TASK_NUM 2

#define SPEEDTEST_COMPTUE_CPU_BASE 1
#define SPEEDTEST_CPU_BASE (SPEEDTEST_COMPTUE_CPU_BASE + 1)

#define SPEEDTEST_SKBQUEUE_SIZE (1024 * 1024)

#define GET_RESIDUAL_BYTE(size, s) (size - (skb_tail_pointer(s) - skb_mac_header(s)))




struct speedtest_status {
	unsigned long begin_time;
	unsigned long pktrcv_count;
	unsigned long pktsend_count;
	unsigned long pkterr_count;
	__sum16 pkt_csum;
};

struct speedtest_cfg {
	int enable;
	unsigned int dur_time;
	unsigned int speed; //Mbit/s
	unsigned int pkt_size;
	char port;
	struct speedtest_status status;
	struct task_struct *task;
	
};

static struct speedtest_cfg st_cfg[SPEEDTEST_TASK_NUM] = {0};
static struct task_struct *compute_task = NULL;
static struct kfifo comp_skbqueue;
static struct timer_list loop_detect_timer;
static int run_speedtest_by_cmd(char *cmd);

static char st0_mac[6] = {0x10,0x10,0x10,0x10,0x10,0x10}; 
static char st1_mac[6] = {0x40,0x40,0x40,0x40,0x40,0x40}; 
static int st_ready = 0;
static int st_loop_timer_enable = 1;

extern int (*hs_speedtest_rx_hook)(struct sk_buff *skb);
extern int (*hs_speedtest_tx_hook)(struct sk_buff *skb);



static int get_speedtest_port_by_index(int i)
{
	return (i ? 4 : 1);
}


static int is_speedtest_task_running(void)
{
	int i = 0;
	if(likely(compute_task))
	{
		return 1;
	}
	for(i = 0; i < SPEEDTEST_TASK_NUM; i++)
	{
		if(likely(st_cfg[i].task))
		{
			return 1;
		}
	}
	return 0;
}



static int xmit_speedtest_pkt(struct sk_buff *skb)
{
	if(likely(is_speedtest_task_running()))
	{
		dev_kfree_skb_any(skb);
		return 0;
	}
	return 1;
}
 
static int fast_rcv_speedtest_pkt(struct sk_buff *skb)
{

	kfifo_in(&comp_skbqueue, &skb, sizeof(void *));
	return 1;
}

static int rcv_speedtest_probe_pkt(struct sk_buff *skb)
{
	struct ethhdr *ethd = (struct ethhdr *)(skb->data);
	
	if(unlikely(!is_speedtest_task_running() && st_loop_timer_enable))
	{
		if(unlikely(!memcmp(ethd->h_source, st0_mac, 6)))
		{
		
			ST_DBG("detect the speedtest loop probe 1...\n");
			st_ready  |= 0x01;
			goto free_probe_skb;
		}
		if(unlikely(!memcmp(ethd->h_source, st1_mac, 6)))
		{
		
			ST_DBG("detect the speedtest loop probe 2...\n");
			st_ready  |= 0x02;
			goto free_probe_skb;

		}
		//forward other packet
		return 0;
	}
	
	return 1;
free_probe_skb:
	if(st_ready == 0x03)
	{
		ST_DBG("detect the speedtest loop, ready to run...\n");
		del_timer_sync(&loop_detect_timer);
	}
	dev_kfree_skb_any(skb);
	return 1;
}

static int make_speedtest_pkt(struct sk_buff **st_skb, char port, int pkt_size)
{

	struct sk_buff *skb = NULL;
	struct net_device *dev = NULL;
	int i = 0;
	unsigned short hlen = 0;
	unsigned short tlen = 0;
	__wsum csum;
	char dev_name[16] = {0};

	struct ethhdr *ethd = NULL;
	struct iphdr *iph = NULL;
	struct udphdr *udph = NULL;
	
	sprintf(dev_name, "eth0.%d", port);
	dev = __dev_get_by_name(&init_net, dev_name);
    if (dev == NULL)
    {
		ST_ERR("get dev name %s failed\n", dev_name);
        return 0;
    }
	
	hlen = LL_RESERVED_SPACE(dev);
	tlen = dev->needed_tailroom;
	
	
	skb = alloc_skb(pkt_size + hlen + tlen, GFP_ATOMIC | __GFP_NOWARN);
	ST_DBG("skb data size is %d\n", pkt_size + hlen + tlen);
	if (!skb)
	{
		ST_ERR("alloc skb failed\n");
		return 0;
	}
	
	skb->dev = dev;
	skb_reserve(skb, hlen);
	skb_reset_mac_header(skb);
	ethd = eth_hdr(skb);
	skb_put(skb, sizeof(struct ethhdr));
	
	
	
	memset(ethd->h_dest, 0x80, 6);
	memset(ethd->h_source, ((port & 0x0f) << 4), 6);
	ethd->h_proto = htons(0x0800);
	skb->network_header = skb->mac_header + sizeof(struct ethhdr);
	iph = ip_hdr(skb);

	skb_put(skb, sizeof(struct iphdr));
	iph->ihl	  = (sizeof(struct iphdr))>>2;
	skb->transport_header = skb->network_header + sizeof(struct iphdr);

	iph->version  = 4;
	iph->tos      = 0;
	iph->frag_off = 0;
	iph->ttl      = 64;
	iph->daddr	  = htonl(0xc0a85502);
	iph->saddr    = htonl(0xc0a85501);
	iph->protocol = IPPROTO_UDP;
	iph->id = 0x1111;
	iph->check = 0;
	
	skb_put(skb, GET_RESIDUAL_BYTE(pkt_size, skb));
	
	udph = udp_hdr(skb);
	udph->source = htons(8888);
	udph->dest = htons(9999);
	udph->len = htons(skb_tail_pointer(skb) - skb_transport_header(skb));
	
	memset((char *)udph+sizeof(struct udphdr), 0x11, ntohs(udph->len) - sizeof(struct udphdr));
	udph->check = 0;
	csum = skb_checksum(skb, skb_transport_offset(skb), ntohs(udph->len), 0);
	udph->check = csum_tcpudp_magic(iph->saddr, iph->daddr,
					ntohs(udph->len), IPPROTO_UDP, csum);
	iph->tot_len  = htons(skb_tail_pointer(skb) - skb_network_header(skb));
	iph->check = ip_compute_csum((char *)iph, sizeof(struct iphdr));
	/*print_hex_dump(KERN_ERR,"speedtest pkt dump:", 
		DUMP_PREFIX_OFFSET, 16, 1, 
		skb_mac_header(skb), skb->len, true);*/
	
	*st_skb = skb;
	return 1;
}

static inline int send_speedtest_pkt(struct sk_buff *st_skb)
{
	struct sk_buff *skb = NULL;

	skb = skb_clone(st_skb, GFP_ATOMIC);
    if (likely(skb))
	{
		if(!skb->dev->netdev_ops->ndo_start_xmit(skb, skb->dev))
		{
			return 1;
		}
		
		kfree_skb(skb);
	}
	
	ST_ERR("send pkt failed\n");
	return 0;
}
static void speedtest_loop_detect_cb (unsigned long arg)
{
	int i = 0;
	static unsigned long ready_endtime = 0;
	struct sk_buff *probe_skb = NULL;
	
	ST_DBG("in detect loop timer...\n");
	
	if(is_speedtest_task_running())
	{
		ST_DBG("task is running, exit timer...\n");
		return;
	}
	
	for(i = 0; i < SPEEDTEST_TASK_NUM; i++)
	{
		if(!make_speedtest_pkt(&probe_skb, get_speedtest_port_by_index(i), 1280))
		{
			ST_DBG("make speedtest skb failed.\n");
		}
		
		if(!send_speedtest_pkt(probe_skb))
		{
			ST_DBG("send probe skb failed.\n");
		}
	}
	
	if(st_ready && !ready_endtime)
	{
	
		ST_DBG("wait ready to go for 5 seconds.\n");
		ready_endtime = jiffies + 5 * HZ;
	}

	if(!ready_endtime && ready_endtime < jiffies)
	{
		ST_DBG("clear ready flag, continue to wait.\n");
		ready_endtime = 0;
		st_ready = 0;
	}

	mod_timer(&loop_detect_timer, jiffies + HZ * SPEEDTEST_LOOPDETECT_TIMER_CIRCLE);

	return;
}

static void speedtest_loopdetect_timer_init(void)
{
	init_timer(&loop_detect_timer);
	loop_detect_timer.function = speedtest_loop_detect_cb;
	loop_detect_timer.data = NULL;
	mod_timer(&loop_detect_timer, jiffies + HZ * SPEEDTEST_LOOPDETECT_TIMER_CIRCLE);

}

static int speedtest_compute_func(void *data)
{
	int i = 0;
	int is_st_running = 0;
	unsigned long end_time = 0;
	struct sk_buff *skb = NULL;
	struct ethhdr *ethd = NULL;
	
	ST_DBG("in compute thread func\n");
	while(1)
	{
		while(kfifo_out(&comp_skbqueue, &skb, sizeof(void *)))
		{
			ethd = (struct ethhdr *)(skb->data);
			if(ethd->h_source[0] == 0x10)
			{
				st_cfg[0].status.pktrcv_count++;
				if(unlikely(st_cfg[0].status.pkt_csum != ip_compute_csum(skb->data + 18, skb->len - 18)))
				{
					st_cfg[0].status.pkterr_count++;
				}
			}
			else if(ethd->h_source[0] == 0x40)
			{
				st_cfg[1].status.pktrcv_count++;
				
				if(unlikely(st_cfg[1].status.pkt_csum != ip_compute_csum(skb->data + 18, skb->len - 18)))
				{
					st_cfg[1].status.pkterr_count++;
				}
			}
			
			dev_kfree_skb_any(skb);
		}
		
		is_st_running = 0;
		for(i = 0; i < SPEEDTEST_TASK_NUM; i++)
		{
			if(st_cfg[i].task)
			{
				is_st_running = 1;
				end_time = 0;
				break;
			}
		}
		
		if(!is_st_running && !end_time)
		{
		
			end_time = jiffies + 5 * HZ;
		}
		if(unlikely(kthread_should_stop() || (!is_st_running && end_time < jiffies)))
		{
			break;
		}		
		schedule();
	}
	while(kfifo_out(&comp_skbqueue, &skb, sizeof(void *)))
	{
		dev_kfree_skb_any(skb);
	}
	compute_task = NULL;
	if(st_loop_timer_enable)
	{
		ST_DBG("enable loop detect timer.\n");
		st_ready = 0;
		speedtest_loopdetect_timer_init();
		hs_speedtest_rx_hook = rcv_speedtest_probe_pkt;
	}
	ST_DBG("user quit compute thread.\n");
	return 0;

}

static int speedtest_thread_func(void *data)
{
	struct sk_buff *st_skb = NULL;
	struct speedtest_cfg *cfg =  (struct speedtest_cfg *)data;
	unsigned long end_time = 0;
	unsigned long speed_time = 0;
	unsigned long current_timens = 0;
	unsigned long send_time = 0;
	struct timespec64 current_time = {0};
	ST_DBG("in speedtest thread func\n");
	
	if(!cfg)
	{
		ST_ERR("thread cfg is NULL\n");
		return 0;
	}
	
	if(!make_speedtest_pkt(&st_skb, cfg->port, cfg->pkt_size))
	{
		ST_DBG("make speedtest skb failed.\n");
		return 0;
	}
	cfg->status.pkt_csum = ip_compute_csum(st_skb->data + 14, st_skb->len - 14);// skip ETH header
	ST_DBG("pkt_csum: 0x%04x, st_skb->len: %d\n", cfg->status.pkt_csum, st_skb->len - 14);



	cfg->status.begin_time = jiffies/HZ;
	end_time = jiffies + cfg->dur_time * HZ;
	send_time = 1000000000 / (cfg->speed * 1024 * 1024 / 8 / cfg->pkt_size);// pkts/ns
	
	__getnstimeofday64(&current_time);
	speed_time = send_time + current_time.tv_sec * 1000000000 + current_time.tv_nsec;
	ST_DBG("calc end_time: %lu, send_time: %lu, speed_time: %lu\n", end_time, send_time, speed_time);
	while(1)
	{
		if(cfg->speed < 200)
		{
			__getnstimeofday64(&current_time);
			current_timens =  current_time.tv_sec * 1000000000 + current_time.tv_nsec;
			if(speed_time >= current_timens)
			{
				continue;
			}
			speed_time = send_time + current_timens;
		}
		if(likely(send_speedtest_pkt(st_skb)))
		{
			cfg->status.pktsend_count++;
		}

		
		if(unlikely(kthread_should_stop() || (end_time < jiffies)))
		{
			break;
		}
		
	}
	
	ST_DBG("user quit speedtest thread.\n");
	kfree_skb(st_skb);
	cfg->task = NULL;
	cfg->enable = 0;
	return 0;
}


static int speedtest_proc_show(struct seq_file *m, void *v)
{
	int i = 0;

	
	seq_printf(m, "SPEEDTEST STATUS: READY_%d\n", st_ready);
	for(i = 0; i < SPEEDTEST_TASK_NUM; i++)
	{
	
		seq_printf(m, "===================  DIRECTION %d  =================\n", i);
		
		seq_printf(m, "STATUS:\n");
		
		seq_printf(m, "       pktsend count:%lu,  pktrcv count:%lu, pkterr count:%lu, used time: %lu, pkt_csum: 0x%08x\n", 
			st_cfg[i].status.pktsend_count, 
			st_cfg[i].status.pktrcv_count,  
			st_cfg[i].status.pkterr_count, jiffies/HZ - st_cfg[i].status.begin_time, 
			st_cfg[i].status.pkt_csum);
		
		seq_printf(m, "CONFIG:\n");
		
		seq_printf(m, "       enable:%d,  dur_time:%u, speed: %u, port: %u\n", 
			st_cfg[i].enable, st_cfg[i].dur_time, st_cfg[i].speed, st_cfg[i].port);
	}

	return 0;
}
static void stop_compspeedtest_task(void)
{
	while(compute_task)
	{
		ST_DBG("stop old compute thread...\n");
		kthread_stop(compute_task);
	}
	return;

}

static void stop_allspeedtest_task(void)
{
	int i = 0;
	for(i = 0; i < SPEEDTEST_TASK_NUM; i++)
	{
		while(st_cfg[i].task)
		{
			ST_DBG("stop old d%d thread...\n", i);
			kthread_stop(st_cfg[i].task);
		}
	}
	return;

}
static void start_allspeedtest_task(void)
{
	int i = 0;
	for(i = 0; i < SPEEDTEST_TASK_NUM; i++)
	{
		ST_DBG("start speedtest-%d thread on cpu: %d\n", i, task_thread_info(st_cfg[i].task)->cpu);
		wake_up_process(st_cfg[i].task);
	}
	return;

}


static int run_speedtest_by_cmd(char *cmd)
{
	int task_run_flag = 0;
	int i = 0;
	struct sched_param param = {
		.sched_priority = 0
	};

	ST_DBG("speedtest_proc_write success get speedtest running cmd: %s...\n", cmd);

	for(i = 0; i < SPEEDTEST_TASK_NUM; i++)
	{
		if(sscanf(cmd, "%d %d %d %d %d", &st_cfg[i].enable, &st_cfg[i].dur_time, &st_cfg[i].speed, &st_cfg[i].pkt_size, &st_loop_timer_enable) != 5)
		{
			ST_ERR("echo [enable:0/1] [dur_time:second] [speed:Mbit/s] [pkt_size:byte] [loop_timer_enable:0/1]\n");
			return 1;
		}
		//datapath config

		st_cfg[i].port = get_speedtest_port_by_index(i);

	}
	

	stop_allspeedtest_task();
	
	//run speedtest stask
	for(i = 0; i < SPEEDTEST_TASK_NUM; i++)
	{
		
		if(st_cfg[i].enable)
		{
			char task_name[16];
			memset((void *) &st_cfg[i].status, 0, sizeof(struct speedtest_status));
			ST_DBG("begin to start a thread to send pkt...\n");
			
			sprintf(task_name, "speedtest-d%d", i);
			st_cfg[i].task = kthread_create(speedtest_thread_func, &st_cfg[i], task_name);
			
			if(IS_ERR(st_cfg[i].task)){
				ST_ERR("Unable to start kernel %s thread.\n", task_name);
				goto out;
			}
			
			sched_setscheduler(st_cfg[i].task, SCHED_FIFO, &param);
			kthread_bind(st_cfg[i].task, SPEEDTEST_CPU_BASE + i); /* bind to CPU2 ~ CPU3 */
			task_run_flag = 1;
			
		}

	}

	if(task_run_flag)
	{
		hs_speedtest_rx_hook = fast_rcv_speedtest_pkt;
		st_ready = 0;
		stop_compspeedtest_task();
		compute_task = kthread_create(speedtest_compute_func, NULL, "speedtest-comp");
		if(IS_ERR(compute_task)){
			ST_ERR("Unable to start kernel compute thread.\n");
			stop_allspeedtest_task();
			goto out;
		}
		sched_setscheduler(compute_task, SCHED_FIFO, &param);
		kthread_bind(compute_task, SPEEDTEST_COMPTUE_CPU_BASE); /* bind to CPU1 */
		wake_up_process(compute_task);
		
		ST_DBG("start speedtest-comp thread on cpu: %d\n", task_thread_info(compute_task)->cpu);
		start_allspeedtest_task();
	}
out:
	return 0;

}

static ssize_t speedtest_proc_write(struct file *file, const char __user *buffer,
				    size_t count, loff_t *pos)
{
	char data[256];
	if (copy_from_user(data, buffer, count)) {
		ST_ERR("Unable to get speedtest data form proc.\n");
		return -EFAULT;
	}
	data[count] = '\0';
	
	run_speedtest_by_cmd(data);


	return count;
}

static int speedtest_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, speedtest_proc_show, NULL);
}

static const struct file_operations speedtest_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= speedtest_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.write		= speedtest_proc_write,
	.release	= single_release,
};

static int __init speedtest_init_module(void)
{
	int st_err;

	if (!proc_create("speedtest", S_IRUGO, init_net.proc_net, &speedtest_proc_fops))
		return -ENOMEM;
	
	if (kfifo_alloc(&comp_skbqueue, SPEEDTEST_SKBQUEUE_SIZE, GFP_KERNEL)) {
		ST_ERR("error kfifo_alloc\n");
		goto err;
	}
	
	hs_speedtest_rx_hook = rcv_speedtest_probe_pkt;
	hs_speedtest_tx_hook = xmit_speedtest_pkt;
	
	speedtest_loopdetect_timer_init();


    return 0;
	
err:
	remove_proc_entry("speedtest", init_net.proc_net);
	return -ENOMEM;
}


static void __exit speedtest_exit_module(void)
{
	del_timer_sync(&loop_detect_timer);
    stop_allspeedtest_task();
	stop_compspeedtest_task();
	remove_proc_entry("speedtest", init_net.proc_net);
	hs_speedtest_rx_hook = NULL;
	hs_speedtest_tx_hook = NULL;
	kfifo_free(&comp_skbqueue);
}
module_init(speedtest_init_module);
module_exit(speedtest_exit_module);


MODULE_LICENSE("GPL v2");

