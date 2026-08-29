#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#include "mini_vuln.h"

static u32 g_val = 100;

static int copy_int_from_user(unsigned long arg, struct mini_vuln_int *out)
{
	if (copy_from_user(out, (void __user *)arg, sizeof(*out)))
		return -EFAULT;
	return 0;
}

static int str_overflow(unsigned long arg)
{
	struct mini_vuln_str req;
	char kbuf[MINI_VULN_KBUF];

	if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
		return -EFAULT;

	if (req.len > MINI_VULN_STR_MAX)
		req.len = MINI_VULN_STR_MAX;

	memset(kbuf, 0, sizeof(kbuf));
	printk(KERN_INFO "mini_vuln: STR len=%u into kbuf[%u]\n",
	       req.len, MINI_VULN_KBUF);
	/* kbuf is 32 bytes; req.len is not checked against MINI_VULN_KBUF. */
	memcpy(kbuf, req.data, req.len);
	return 0;
}

static long mini_vuln_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct mini_vuln_int req = {0};
	u32 before = g_val;

	switch (cmd) {
	case MINI_VULN_GET:
		req.value = g_val;
		if (copy_to_user((void __user *)arg, &req, sizeof(req)))
			return -EFAULT;
		printk(KERN_INFO "mini_vuln: GET %u (0x%x)\n", g_val, g_val);
		return 0;

	case MINI_VULN_SET:
		if (copy_int_from_user(arg, &req))
			return -EFAULT;
		g_val = req.value;
		printk(KERN_INFO "mini_vuln: SET %u -> %u\n", before, g_val);
		return 0;

	case MINI_VULN_ADD:
		if (copy_int_from_user(arg, &req))
			return -EFAULT;
		g_val = g_val + req.value;
		printk(KERN_INFO "mini_vuln: ADD %u + %u = %u (wrap=%s)\n",
		       before, req.value, g_val,
		       (g_val < before) ? "yes" : "no");
		return 0;

	case MINI_VULN_SUB:
		if (copy_int_from_user(arg, &req))
			return -EFAULT;
		g_val = g_val - req.value;
		printk(KERN_INFO "mini_vuln: SUB %u - %u = %u (wrap=%s)\n",
		       before, req.value, g_val,
		       (g_val > before) ? "yes" : "no");
		return 0;

	case MINI_VULN_STR:
		return str_overflow(arg);

	default:
		return -ENOTTY;
	}
}

static const struct file_operations mini_vuln_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = mini_vuln_ioctl,
};

static struct miscdevice mini_vuln_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = MINI_VULN_NAME,
	.fops = &mini_vuln_fops,
	.mode = 0666,
};

static int __init mini_vuln_init(void)
{
	int ret = misc_register(&mini_vuln_dev);

	if (ret)
		return ret;
	printk(KERN_INFO "mini_vuln: loaded, /dev/%s\n", MINI_VULN_NAME);
	return 0;
}

static void __exit mini_vuln_exit(void)
{
	misc_deregister(&mini_vuln_dev);
	printk(KERN_INFO "mini_vuln: unloaded\n");
}

module_init(mini_vuln_init);
module_exit(mini_vuln_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Damian");
MODULE_DESCRIPTION("Educational integer wrap + string buffer overflow lab");
