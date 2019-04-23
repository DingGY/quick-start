/*********************************************************************************
 *      Copyright:  (C) 2018.6.2 hisense
 *                  All rights reserved.
 *
 *       Filename:  board_button_dev.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(2018.6.2)
 *         Author:  dingguangyu
 *      ChangeLog:  1, Release initial version on "2018.6.2"
 *                 
 ********************************************************************************/


#include <linux/init.h>
#include <linux/module.h>


#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/poll.h>
#include "board_input_dev.h"

#define DEBUG
#ifdef DEBUG
#define BRD_BTN_DEBUG(fmt, arg...) printk(fmt, ##arg)
#else
#define BRD_BTN_DEBUG(fmt, arg...) 
#endif

#define DEVICE_NAME "board_event"
static dev_t dev_id;
static struct class * board_button_class;
static struct cdev cdev;


static wait_queue_head_t board_button_wait_queue;

static board_button_data_t send_data;
static board_button_data_t recv_data;
static int trigger_status = 0;


//HGUB963XX-41 dingguangyu modify for key factory  funcation 2018.6.26 begin


void  set_trigger_board_input_event_handler(void *data, int len)
{

	if(len > BOARD_BUTTON_SEND_BUF_SIZE)
		return;
	send_data.len = len;
	memcpy(send_data.data, data, send_data.len);
	printk("trigger_board_input_event_handler: data: %s, len: %lu\n", send_data.data, send_data.len);
	wake_up(&board_button_wait_queue);		
	trigger_status = 1;
}
EXPORT_SYMBOL(set_trigger_board_input_event_handler);




int  get_trigger_board_input_event_handler(void *data, int len)
{
	if(len < recv_data.len)
		return 0;
	memcpy(data, recv_data.data, recv_data.len);
	printk("trigger_board_input_event_handler: data: %s, len: %lu\n", recv_data.data, recv_data.len);
	return recv_data.len;
}
EXPORT_SYMBOL(get_trigger_board_input_event_handler);

//HGUB963XX-41 dingguangyu modify for key factory  funcation 2018.6.26 end



static unsigned int board_button_poll (struct file * filp, struct poll_table_struct * wait)
{
	unsigned int mask = 0;
	poll_wait(filp, &board_button_wait_queue, wait);
	BRD_BTN_DEBUG("board_button poll\n");
	if (trigger_status)
	{
		mask |= POLLIN | POLLRDNORM;
		trigger_status = 0;
	}
	return mask;
}



static ssize_t board_button_read(struct file * pfile, char __user * buf, size_t size, loff_t * ppos)
{
	BRD_BTN_DEBUG("board_button_read read\n");
	if(size > BOARD_BUTTON_SEND_BUF_SIZE)
		goto fail;
	if (copy_to_user(buf, send_data.data, send_data.len))
	{
		BRD_BTN_DEBUG("board_button_read read error\n");
		goto fail;
	}
	return send_data.len;	
fail:
	return -EFAULT;

}


static ssize_t board_button_write(struct file * pfile, const char __user * buf, size_t count, loff_t * ppos)
{
	if (count > BOARD_BUTTON_RECV_BUF_SIZE)
	{
		goto fail;
	}
	if (!copy_from_user(recv_data.data, buf, count))
	{
		recv_data.len = count;
		wake_up(&board_button_wait_queue);		
		BRD_BTN_DEBUG("board_button_write: write, data: %s, len: %lu\n", recv_data.data, recv_data.len);
	}
	return count;
fail:
	return -EFAULT;
}

static int board_button_open(struct inode * pnode, struct file * pfile)
{
	BRD_BTN_DEBUG("board_button_open: open board button\n");
	return 0;
}


static int board_button_release(struct inode * pnode, struct file * pfile)
{
	BRD_BTN_DEBUG("board_button_release: close board button\n");
	return 0;
}


static struct file_operations board_button_opt = 
{
	.owner = THIS_MODULE,
	.read = board_button_read,
	.open = board_button_open,
	.release = board_button_release,
	.write = board_button_write,
	.poll = board_button_poll,
};


static int __init board_button_dev_init(void)
{
	if (alloc_chrdev_region(&dev_id, 0, 1, DEVICE_NAME))
	{
		
		BRD_BTN_DEBUG("board_button_dev_init: alloc_chrdev_region failed\n");
		goto fail;
	}

	recv_data.data = kmalloc(BOARD_BUTTON_RECV_BUF_SIZE, GFP_KERNEL);
	send_data.data = kmalloc(BOARD_BUTTON_SEND_BUF_SIZE, GFP_KERNEL);
	if (recv_data.data == NULL || send_data.data == NULL) 
	{
		BRD_BTN_DEBUG("board_button_dev_init: kmalloc failed\n");
		goto fail;
	}
	recv_data.len = 0;
	send_data.len = 0;
	cdev_init(&cdev, &board_button_opt);
 	if(cdev_add(&cdev, dev_id, 1)) 
	{			
		BRD_BTN_DEBUG("board_button_dev_init: cdev_add failed\n");	
		goto fail;
	}
	
	board_button_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(board_button_class)) {
		BRD_BTN_DEBUG("board_button_dev_init: class_create failed\n");	
		goto err_cdev_del;
	}
	
	if (IS_ERR(device_create(board_button_class, NULL, dev_id, NULL, DEVICE_NAME))) {
		BRD_BTN_DEBUG("board_button_dev_init: device_create failed\n");	
		goto err_class_destory;
	}

	init_waitqueue_head(&board_button_wait_queue);
	BRD_BTN_DEBUG(" init board_button\n");
	return 0;
err_class_destory:
	class_destroy(board_button_class);
err_cdev_del:
	cdev_del(&cdev);
fail:
	kfree(recv_data.data);
	kfree(send_data.data);
	unregister_chrdev_region(dev_id, 1);
	return -EFAULT;

}


static void __exit board_button_dev_exit(void)
{
	kfree(recv_data.data);
	kfree(send_data.data);
	device_destroy(board_button_class, dev_id);
	class_destroy(board_button_class);
	cdev_del(&cdev);
	unregister_chrdev_region(dev_id, 1);
	BRD_BTN_DEBUG(" cleanup board_button_dev_exit\n");
}


module_init(board_button_dev_init);
module_exit(board_button_dev_exit);
MODULE_DESCRIPTION("bcm board button input driver platform_driver");
MODULE_AUTHOR("dingguangyu");
MODULE_LICENSE("GPL");

