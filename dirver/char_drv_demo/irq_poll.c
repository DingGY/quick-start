#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>


#include <linux/irq.h>

#include <asm/irq.h>

#include <mach/regs-gpio.h>
#include <linux/poll.h>
#include <linux/interrupt.h>

#include <linux/init.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include <linux/regulator/consumer.h>
#include <linux/delay.h>

#include <linux/semaphore.h>
#include <linux/sched.h>

#define DRIVER_NAME "DingGY_dev"
#define DEV_NODE "m_key"
#define DEVICE_MINOR_NUM 2
#define DEV_MAJOR 0
#define DEV_MINOR 0
#define BUF_SIZE 100

struct pin_desc{
    unsigned int pin;
    unsigned int val;
};

static struct pin_desc pin_key[] = {
    {EXYNOS4_GPX1(1), 1},
};

static unsigned char key_val = 0;
static volatile unsigned int ev_press = 0;

static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

struct class *pkey_class = NULL;
struct class_device *pkey_class_dev = NULL;
static int major;


static irqreturn_t eint9_interrupt(int irq, void *dev_id) {
    struct pin_desc *pin = (struct pin_desc *)dev_id;

    wake_up_interruptible(&button_waitq);
    return IRQ_HANDLED;
}


static int key_open(struct inode *pnode, struct file *pfile){
    int ret;
    ret = request_irq(IRQ_EINT(9), eint9_interrupt, IRQF_SHARED|IRQ_TYPE_EDGE_BOTH, "key_home", &pin_key);
    return 0;
}

static ssize_t key_write(struct file *file, const char __user *buf,
        size_t count, loff_t *ppos){

    return 0;
}
static int key_release(struct inode *pnode, struct file *pfile){
    free_irq(IRQ_EINT(9),&pin_key);
    return 0;
}


static unsigned key_poll(struct file *file, poll_table *wait){
	unsigned int mask = 0;
	poll_wait(file, &button_waitq, wait);// 不会立即休眠

	if (ev_press)
		mask |= POLLIN | POLLRDNORM;
	return mask;

}
ssize_t key_read(struct file *file, char __user *buf, size_t size, loff_t *ppos){
    int ret;

    if (size != 1)
		return -EINVAL;
    wait_event_interruptible(button_waitq, ev_press);
    ret = copy_to_user(buf, &key_val, 1);
    return 1;
}



struct file_operations key_fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .read = key_read,
    .write = key_write,
    .poll = key_poll,
    .release = key_release,
};


static int __init m_key_init(void){
    major = register_chrdev(DEV_MAJOR, DRIVER_NAME, &key_fops);
    pkey_class = class_create(THIS_MODULE, DRIVER_NAME);
    device_create(pkey_class, NULL, MKDEV(major, 0), NULL, DEV_NODE);
    return 0;
}

static void __exit m_key_exit(void){
    unregister_chrdev(major, DRIVER_NAME);
    device_destroy(pkey_class, MKDEV(major, 0));
    class_destroy(pkey_class);
    return;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DingGY");

module_init(m_key_init);
module_exit(m_key_exit);
