#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mini_vuln.h"

#define DEV_PATH "/dev/" MINI_VULN_NAME

static const char b64tab[] =
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

		putchar(b64tab[(triple >> 18) & 63]);
		putchar(b64tab[(triple >> 12) & 63]);
		putchar((i + 1 < n) ? b64tab[(triple >> 6) & 63] : '=');
		putchar((i + 2 < n) ? b64tab[triple & 63] : '=');
	}
}

static void show_int(const char *op, unsigned int cmd,
		     const struct mini_vuln_int *arg, int sending)
{
	printf("\n--- %s ---\n", op);
	printf("ioctl cmd: 0x%x\n", cmd);
	printf("number %s: %u (0x%08x)\n",
	       sending ? "sent" : "buffer", arg->value, arg->value);
	printf("payload b64: ");
	print_b64(arg, sizeof(*arg));
	printf("\n");
}

static void show_str(unsigned int cmd, const struct mini_vuln_str *arg)
{
	printf("\n--- STR ---\n");
	printf("ioctl cmd: 0x%x\n", cmd);
	printf("string sent: \"%s\"\n", arg->data);
	printf("len sent: %u\n", arg->len);
	printf("payload b64: ");
	print_b64(arg, sizeof(*arg));
	printf("\n");
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
	puts("\nmini_vuln CLI");
	puts("  1) GET integer");
	puts("  2) SET integer");
	puts("  3) ADD (u32 overflow wrap)");
	puts("  4) SUB (u32 underflow wrap)");
	puts("  5) STR (copy into 32-byte kernel buffer)");
	puts("  q) quit");
	printf("> ");
	fflush(stdout);
}

int main(void)
{
	int fd;
	char line[32];
	struct mini_vuln_int iarg;
	struct mini_vuln_str sarg;

	fd = open(DEV_PATH, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open %s: %s\n", DEV_PATH, strerror(errno));
		return 1;
	}

	while (1) {
		print_menu();
		if (!fgets(line, sizeof(line), stdin))
			break;

		memset(&iarg, 0, sizeof(iarg));
		memset(&sarg, 0, sizeof(sarg));

		switch (line[0]) {
		case '1':
			show_int("GET", MINI_VULN_GET, &iarg, 0);
			if (ioctl(fd, MINI_VULN_GET, &iarg) < 0) {
				printf("ioctl failed: %s\n", strerror(errno));
				break;
			}
			printf("kernel value: %u (0x%08x)\n", iarg.value, iarg.value);
			printf("returned b64: ");
			print_b64(&iarg, sizeof(iarg));
			printf("\n");
			break;
		case '2':
			if (read_u32("value to SET (dec or 0xhex): ", &iarg.value))
				puts("bad number");
			else {
				show_int("SET", MINI_VULN_SET, &iarg, 1);
				if (ioctl(fd, MINI_VULN_SET, &iarg) < 0)
					printf("ioctl failed: %s\n", strerror(errno));
				else
					puts("ioctl ok");
			}
			break;
		case '3':
			if (read_u32("value to ADD (dec or 0xhex): ", &iarg.value))
				puts("bad number");
			else {
				show_int("ADD", MINI_VULN_ADD, &iarg, 1);
				if (ioctl(fd, MINI_VULN_ADD, &iarg) < 0)
					printf("ioctl failed: %s\n", strerror(errno));
				else
					puts("ioctl ok");
			}
			break;
		case '4':
			if (read_u32("value to SUB (dec or 0xhex): ", &iarg.value))
				puts("bad number");
			else {
				show_int("SUB", MINI_VULN_SUB, &iarg, 1);
				if (ioctl(fd, MINI_VULN_SUB, &iarg) < 0)
					printf("ioctl failed: %s\n", strerror(errno));
				else
					puts("ioctl ok");
			}
			break;
		case '5': {
			char buf[MINI_VULN_STR_MAX];
			size_t n;

			printf("string to send: ");
			fflush(stdout);
			if (!fgets(buf, sizeof(buf), stdin))
				break;
			n = strlen(buf);
			if (n && buf[n - 1] == '\n')
				buf[--n] = '\0';
			sarg.len = (uint32_t)n;
			memcpy(sarg.data, buf, n);
			show_str(MINI_VULN_STR, &sarg);
			if (ioctl(fd, MINI_VULN_STR, &sarg) < 0)
				printf("ioctl failed: %s\n", strerror(errno));
			else
				puts("ioctl ok");
			break;
		}
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
