#ifndef MINI_VULN_H
#define MINI_VULN_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
typedef uint32_t __u32;
#endif

#define MINI_VULN_NAME "mini_vuln"
#define MINI_VULN_IOC_MAGIC 0x1B
#define MINI_VULN_STR_MAX 256
#define MINI_VULN_KBUF 32

struct mini_vuln_int {
	__u32 value;
};

struct mini_vuln_str {
	__u32 len;
	char data[MINI_VULN_STR_MAX];
};

#define MINI_VULN_GET _IOR(MINI_VULN_IOC_MAGIC, 1, struct mini_vuln_int)
#define MINI_VULN_SET _IOW(MINI_VULN_IOC_MAGIC, 2, struct mini_vuln_int)
#define MINI_VULN_ADD _IOW(MINI_VULN_IOC_MAGIC, 3, struct mini_vuln_int)
#define MINI_VULN_SUB _IOW(MINI_VULN_IOC_MAGIC, 4, struct mini_vuln_int)
#define MINI_VULN_STR _IOW(MINI_VULN_IOC_MAGIC, 5, struct mini_vuln_str)

#endif
