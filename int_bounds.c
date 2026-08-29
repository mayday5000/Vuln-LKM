#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#include "int_bounds.h"

/* Educational lab: g_val wraps on ADD/SUB. No overflow/underflow checks. */
static u32 g_val = 100;

static int copy_arg_from_user(unsigned long arg, struct int_bounds_arg *out)
{
	if (copy_from_user(out, (void __user *)arg, sizeof(*out)))
		return -EFAULT;
	return 0;
}

static long int_bounds_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct int_bounds_arg req = {0};
	u32 before = g_val;

	switch (cmd) {
	case INT_BOUNDS_GET:
		req.value = g_val;
		if (copy_to_user((void __user *)arg, &req, sizeof(req)))
			return -EFAULT;
		printk(KERN_INFO "int_bounds: GET %u (0x%x)\n", g_val, g_val);
		return 0;

	case INT_BOUNDS_SET:
		if (copy_arg_from_user(arg, &req))
			return -EFAULT;
		g_val = req.value;
		printk(KERN_INFO "int_bounds: SET %u -> %u\n", before, g_val);
		return 0;

	case INT_BOUNDS_ADD:
		if (copy_arg_from_user(arg, &req))
			return -EFAULT;
		g_val = g_val + req.value;
		printk(KERN_INFO "int_bounds: ADD %u + %u = %u (wrap=%s)\n",
		       before, req.value, g_val,
		       (g_val < before) ? "yes" : "no");
		return 0;

	case INT_BOUNDS_SUB:
		if (copy_arg_from_user(arg, &req))
			return -EFAULT;
		g_val = g_val - req.value;
		printk(KERN_INFO "int_bounds: SUB %u - %u = %u (wrap=%s)\n",
		       before, req.value, g_val,
		       (g_val > before) ? "yes" : "no");
		return 0;

	default:
		return -ENOTTY;
	}
}

static const struct file_operations int_bounds_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = int_bounds_ioctl,
};

static struct miscdevice int_bounds_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = INT_BOUNDS_NAME,
	.fops = &int_bounds_fops,
	.mode = 0666,
};

static int __init int_bounds_init(void)
{
	int ret = misc_register(&int_bounds_dev);

	if (ret)
		return ret;
	printk(KERN_INFO "int_bounds: loaded, /dev/%s, g_val=%u\n",
	       INT_BOUNDS_NAME, g_val);
	return 0;
}

static void __exit int_bounds_exit(void)
{
	misc_deregister(&int_bounds_dev);
	printk(KERN_INFO "int_bounds: unloaded\n");
}

module_init(int_bounds_init);
module_exit(int_bounds_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Damian");
MODULE_DESCRIPTION("Educational integer overflow/underflow ioctl lab");
