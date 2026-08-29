#ifndef VULN_LKM_H
#define VULN_LKM_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
typedef uint32_t __u32;
#endif

#define VULN_LKM_NAME "vuln_lkm"
#define VULN_LKM_IOC_MAGIC 0x1B
#define VULN_LKM_STR_MAX 256
#define VULN_LKM_KBUF 32

struct vuln_lkm_int {
	__u32 value;
};

struct vuln_lkm_str {
	__u32 len;
	char data[VULN_LKM_STR_MAX];
};

#define VULN_LKM_GET _IOR(VULN_LKM_IOC_MAGIC, 1, struct vuln_lkm_int)
#define VULN_LKM_SET _IOW(VULN_LKM_IOC_MAGIC, 2, struct vuln_lkm_int)
#define VULN_LKM_ADD _IOW(VULN_LKM_IOC_MAGIC, 3, struct vuln_lkm_int)
#define VULN_LKM_SUB _IOW(VULN_LKM_IOC_MAGIC, 4, struct vuln_lkm_int)
#define VULN_LKM_STR _IOW(VULN_LKM_IOC_MAGIC, 5, struct vuln_lkm_str)

#endif
