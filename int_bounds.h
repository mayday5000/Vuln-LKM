#ifndef INT_BOUNDS_H
#define INT_BOUNDS_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
typedef uint32_t __u32;
#endif

#define INT_BOUNDS_NAME "int_bounds"
#define INT_BOUNDS_IOC_MAGIC 0x1B

struct int_bounds_arg {
	__u32 value;
};

#define INT_BOUNDS_GET _IOR(INT_BOUNDS_IOC_MAGIC, 1, struct int_bounds_arg)
#define INT_BOUNDS_SET _IOW(INT_BOUNDS_IOC_MAGIC, 2, struct int_bounds_arg)
#define INT_BOUNDS_ADD _IOW(INT_BOUNDS_IOC_MAGIC, 3, struct int_bounds_arg)
#define INT_BOUNDS_SUB _IOW(INT_BOUNDS_IOC_MAGIC, 4, struct int_bounds_arg)

#endif
