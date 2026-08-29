#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#include "vuln_lkm.h"

static u32 g_val = 100;

static int copy_int_from_user(unsigned long arg, struct vuln_lkm_int *out)
{
	if (copy_from_user(out, (void __user *)arg, sizeof(*out)))
		return -EFAULT;
	return 0;
}

/* GDB: break vuln_lkm_add */
static int vuln_lkm_add(u32 n)
{
	u32 before = g_val;

	/* No check against U32_MAX - n. The add wraps. */
	g_val = g_val + n;
	printk(KERN_INFO "vuln_lkm: ADD %u + %u = %u (wrap=%s)\n",
	       before, n, g_val, (g_val < before) ? "yes" : "no");
	return 0;
}

/* GDB: break vuln_lkm_sub */
static int vuln_lkm_sub(u32 n)
{
	u32 before = g_val;

	/* No check against n > g_val. The sub wraps. */
	g_val = g_val - n;
	printk(KERN_INFO "vuln_lkm: SUB %u - %u = %u (wrap=%s)\n",
	       before, n, g_val, (g_val > before) ? "yes" : "no");
	return 0;
}

/* GDB: break vuln_lkm_str */
static int vuln_lkm_str(unsigned long arg)
{
	struct vuln_lkm_str req;
	char kbuf[VULN_LKM_KBUF];

	if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
		return -EFAULT;

	if (req.len > VULN_LKM_STR_MAX)
		req.len = VULN_LKM_STR_MAX;

	memset(kbuf, 0, sizeof(kbuf));
	printk(KERN_INFO "vuln_lkm: STR len=%u into kbuf[%u]\n",
	       req.len, VULN_LKM_KBUF);
	/*
	 * kbuf is 32 bytes. req.len is not compared to VULN_LKM_KBUF.
	 * memcpy uses the user-supplied length as-is.
	 */
	memcpy(kbuf, req.data, req.len);
	return 0;
}

static long vuln_lkm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct vuln_lkm_int req = {0};

	switch (cmd) {
	case VULN_LKM_GET:
		req.value = g_val;
		if (copy_to_user((void __user *)arg, &req, sizeof(req)))
			return -EFAULT;
		printk(KERN_INFO "vuln_lkm: GET %u (0x%x)\n", g_val, g_val);
		return 0;

	case VULN_LKM_SET:
		if (copy_int_from_user(arg, &req))
			return -EFAULT;
		printk(KERN_INFO "vuln_lkm: SET %u -> %u\n", g_val, req.value);
		g_val = req.value;
		return 0;

	case VULN_LKM_ADD:
		if (copy_int_from_user(arg, &req))
			return -EFAULT;
		return vuln_lkm_add(req.value);

	case VULN_LKM_SUB:
		if (copy_int_from_user(arg, &req))
			return -EFAULT;
		return vuln_lkm_sub(req.value);

	case VULN_LKM_STR:
		return vuln_lkm_str(arg);

	default:
		return -ENOTTY;
	}
}

static const struct file_operations vuln_lkm_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = vuln_lkm_ioctl,
};

static struct miscdevice vuln_lkm_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = VULN_LKM_NAME,
	.fops = &vuln_lkm_fops,
	.mode = 0666,
};

static int __init vuln_lkm_init(void)
{
	int ret = misc_register(&vuln_lkm_dev);

	if (ret)
		return ret;
	printk(KERN_INFO "vuln_lkm: loaded, /dev/%s, g_val=%u\n",
	       VULN_LKM_NAME, g_val);
	return 0;
}

static void __exit vuln_lkm_exit(void)
{
	misc_deregister(&vuln_lkm_dev);
	printk(KERN_INFO "vuln_lkm: unloaded\n");
}

module_init(vuln_lkm_init);
module_exit(vuln_lkm_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Damian");
MODULE_DESCRIPTION("Educational LKM: integer wrap and string buffer overflow");
