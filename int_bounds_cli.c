#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "int_bounds.h"

#define DEV_PATH "/dev/" INT_BOUNDS_NAME

static const char b64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void print_b64(const void *data, size_t n)
{
	const unsigned char *p = data;
	size_t i;

	for (i = 0; i < n; i += 3) {
		unsigned a = p[i];
		unsigned b = (i + 1 < n) ? p[i + 1] : 0;
		unsigned c = (i + 2 < n) ? p[i + 2] : 0;
		unsigned triple = (a << 16) | (b << 8) | c;

		putchar(b64[(triple >> 18) & 63]);
		putchar(b64[(triple >> 12) & 63]);
		putchar((i + 1 < n) ? b64[(triple >> 6) & 63] : '=');
		putchar((i + 2 < n) ? b64[triple & 63] : '=');
	}
}

static void show_payload(const char *op, unsigned int cmd,
			 const struct int_bounds_arg *arg, int sending)
{
	printf("\n--- %s ---\n", op);
	printf("ioctl cmd: 0x%x\n", cmd);
	if (sending)
		printf("number sent: %u (0x%08x)\n", arg->value, arg->value);
	else
		printf("number (buffer): %u (0x%08x)\n", arg->value, arg->value);
	printf("payload bytes: %zu\n", sizeof(*arg));
	printf("payload b64: ");
	print_b64(arg, sizeof(*arg));
	printf("\n");
}

static int do_ioctl(int fd, const char *op, unsigned int cmd,
		    struct int_bounds_arg *arg, int sending)
{
	int rc;

	show_payload(op, cmd, arg, sending);
	rc = ioctl(fd, cmd, arg);
	if (rc < 0) {
		printf("ioctl failed: %s\n", strerror(errno));
		return -1;
	}
	printf("ioctl ok\n");
	return 0;
}

static int read_u32(const char *prompt, uint32_t *out)
{
	unsigned long v;
	char buf[64];
	char *end;

	printf("%s", prompt);
	fflush(stdout);
	if (!fgets(buf, sizeof(buf), stdin))
		return -1;
	errno = 0;
	v = strtoul(buf, &end, 0);
	if (end == buf || errno)
		return -1;
	*out = (uint32_t)v;
	return 0;
}

static void print_menu(void)
{
	puts("\nint_bounds CLI");
	puts("  1) GET current kernel value");
	puts("  2) SET value");
	puts("  3) ADD (overflow wraps u32)");
	puts("  4) SUB (underflow wraps u32)");
	puts("  q) quit");
	printf("> ");
	fflush(stdout);
}

int main(void)
{
	int fd;
	char line[32];
	struct int_bounds_arg arg;

	fd = open(DEV_PATH, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open %s: %s\n", DEV_PATH, strerror(errno));
		return 1;
	}

	while (1) {
		print_menu();
		if (!fgets(line, sizeof(line), stdin))
			break;

		memset(&arg, 0, sizeof(arg));

		switch (line[0]) {
		case '1':
			if (do_ioctl(fd, "GET", INT_BOUNDS_GET, &arg, 0) == 0) {
				printf("kernel value: %u (0x%08x)\n",
				       arg.value, arg.value);
				printf("returned b64: ");
				print_b64(&arg, sizeof(arg));
				printf("\n");
			}
			break;
		case '2':
			if (read_u32("value to SET (dec or 0xhex): ", &arg.value))
				puts("bad number");
			else
				do_ioctl(fd, "SET", INT_BOUNDS_SET, &arg, 1);
			break;
		case '3':
			if (read_u32("value to ADD (dec or 0xhex): ", &arg.value))
				puts("bad number");
			else
				do_ioctl(fd, "ADD", INT_BOUNDS_ADD, &arg, 1);
			break;
		case '4':
			if (read_u32("value to SUB (dec or 0xhex): ", &arg.value))
				puts("bad number");
			else
				do_ioctl(fd, "SUB", INT_BOUNDS_SUB, &arg, 1);
			break;
		case 'q':
		case 'Q':
			close(fd);
			return 0;
		default:
			puts("unknown option");
			break;
		}
	}

	close(fd);
	return 0;
}
