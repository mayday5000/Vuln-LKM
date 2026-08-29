#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "vuln_lkm.h"

#define DEV_PATH "/dev/" VULN_LKM_NAME

static const char b64tab[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void print_help(const char *argv0)
{
	printf(
"vuln_lkm_cli — userspace tester for the vuln_lkm educational LKM\n"
"\n"
"Usage:\n"
"  %s                 interactive menu\n"
"  %s -h|--help       this help\n"
"  %s get             VULN_LKM_GET\n"
"  %s set <n>         VULN_LKM_SET\n"
"  %s add <n>         VULN_LKM_ADD  (u32 wrap)\n"
"  %s sub <n>         VULN_LKM_SUB  (u32 wrap)\n"
"  %s str <string>    VULN_LKM_STR  (copy into 32-byte kbuf)\n"
"\n"
"<n> is decimal or 0xhex.\n"
"Every ioctl prints cmd hex, the number or string, and base64 of the payload.\n"
"Device: %s\n"
"\n"
"Examples:\n"
"  %s get\n"
"  %s set 10\n"
"  %s sub 11          # 10-11 wraps to 4294967295\n"
"  %s set 0xfffffffe\n"
"  %s add 3           # wraps to 1\n"
"  %s str hello       # fits in 32-byte kbuf\n"
"  %s str AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA  # 40 bytes > 32\n"
"\n"
"Interactive keys: 1 GET  2 SET  3 ADD  4 SUB  5 STR  q quit\n",
		argv0, argv0, argv0, argv0, argv0, argv0, argv0,
		DEV_PATH,
		argv0, argv0, argv0, argv0, argv0, argv0, argv0);
}

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
		     const struct vuln_lkm_int *arg, int sending)
{
	printf("\n--- %s ---\n", op);
	printf("ioctl cmd: 0x%x\n", cmd);
	printf("number %s: %u (0x%08x)\n",
	       sending ? "sent" : "buffer", arg->value, arg->value);
	printf("payload b64: ");
	print_b64(arg, sizeof(*arg));
	printf("\n");
}

static void show_str(unsigned int cmd, const struct vuln_lkm_str *arg)
{
	printf("\n--- STR ---\n");
	printf("ioctl cmd: 0x%x\n", cmd);
	printf("string sent: \"%s\"\n", arg->data);
	printf("len sent: %u\n", arg->len);
	printf("payload b64: ");
	print_b64(arg, sizeof(*arg));
	printf("\n");
}

static int do_int(int fd, const char *op, unsigned int cmd,
		  struct vuln_lkm_int *arg, int sending)
{
	show_int(op, cmd, arg, sending);
	if (ioctl(fd, cmd, arg) < 0) {
		printf("ioctl failed: %s\n", strerror(errno));
		return -1;
	}
	printf("ioctl ok\n");
	return 0;
}

static int parse_u32(const char *s, uint32_t *out)
{
	unsigned long v;
	char *end;

	errno = 0;
	v = strtoul(s, &end, 0);
	if (end == s || *end != '\0' || errno)
		return -1;
	*out = (uint32_t)v;
	return 0;
}

static int read_u32(const char *prompt, uint32_t *out)
{
	char buf[64];

	printf("%s", prompt);
	fflush(stdout);
	if (!fgets(buf, sizeof(buf), stdin))
		return -1;
	buf[strcspn(buf, "\n")] = '\0';
	return parse_u32(buf, out);
}

static int open_dev(void)
{
	int fd = open(DEV_PATH, O_RDWR);

	if (fd < 0)
		fprintf(stderr, "open %s: %s\n", DEV_PATH, strerror(errno));
	return fd;
}

static int cmd_get(int fd)
{
	struct vuln_lkm_int arg = {0};

	if (do_int(fd, "GET", VULN_LKM_GET, &arg, 0) == 0) {
		printf("kernel value: %u (0x%08x)\n", arg.value, arg.value);
		printf("returned b64: ");
		print_b64(&arg, sizeof(arg));
		printf("\n");
	}
	return 0;
}

static int cmd_set(int fd, uint32_t v)
{
	struct vuln_lkm_int arg = { .value = v };

	return do_int(fd, "SET", VULN_LKM_SET, &arg, 1);
}

static int cmd_add(int fd, uint32_t v)
{
	struct vuln_lkm_int arg = { .value = v };

	return do_int(fd, "ADD", VULN_LKM_ADD, &arg, 1);
}

static int cmd_sub(int fd, uint32_t v)
{
	struct vuln_lkm_int arg = { .value = v };

	return do_int(fd, "SUB", VULN_LKM_SUB, &arg, 1);
}

static int cmd_str(int fd, const char *s)
{
	struct vuln_lkm_str arg;
	size_t n = strlen(s);

	memset(&arg, 0, sizeof(arg));
	if (n > VULN_LKM_STR_MAX)
		n = VULN_LKM_STR_MAX;
	arg.len = (uint32_t)n;
	memcpy(arg.data, s, n);
	show_str(VULN_LKM_STR, &arg);
	if (ioctl(fd, VULN_LKM_STR, &arg) < 0) {
		printf("ioctl failed: %s\n", strerror(errno));
		return -1;
	}
	printf("ioctl ok\n");
	return 0;
}

static int interactive(int fd)
{
	char line[32];
	uint32_t v;
	char buf[VULN_LKM_STR_MAX];

	while (1) {
		puts("\nvuln_lkm CLI");
		puts("  1) GET integer");
		puts("  2) SET integer");
		puts("  3) ADD (u32 overflow wrap)");
		puts("  4) SUB (u32 underflow wrap)");
		puts("  5) STR (copy into 32-byte kernel buffer)");
		puts("  h) help");
		puts("  q) quit");
		printf("> ");
		fflush(stdout);
		if (!fgets(line, sizeof(line), stdin))
			break;
		switch (line[0]) {
		case '1':
			cmd_get(fd);
			break;
		case '2':
			if (read_u32("value to SET (dec or 0xhex): ", &v))
				puts("bad number");
			else
				cmd_set(fd, v);
			break;
		case '3':
			if (read_u32("value to ADD (dec or 0xhex): ", &v))
				puts("bad number");
			else
				cmd_add(fd, v);
			break;
		case '4':
			if (read_u32("value to SUB (dec or 0xhex): ", &v))
				puts("bad number");
			else
				cmd_sub(fd, v);
			break;
		case '5': {
			size_t n;

			printf("string to send: ");
			fflush(stdout);
			if (!fgets(buf, sizeof(buf), stdin))
				break;
			n = strlen(buf);
			if (n && buf[n - 1] == '\n')
				buf[--n] = '\0';
			cmd_str(fd, buf);
			break;
		}
		case 'h':
		case 'H':
			print_help("vuln_lkm_cli");
			break;
		case 'q':
		case 'Q':
			return 0;
		default:
			puts("unknown option (h for help)");
			break;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	int fd, rc = 0;
	uint32_t v;

	if (argc >= 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
		print_help(argv[0]);
		return 0;
	}

	fd = open_dev();
	if (fd < 0)
		return 1;

	if (argc == 1) {
		rc = interactive(fd);
	} else if (!strcmp(argv[1], "get") && argc == 2) {
		rc = cmd_get(fd);
	} else if (!strcmp(argv[1], "set") && argc == 3 && !parse_u32(argv[2], &v)) {
		rc = cmd_set(fd, v);
	} else if (!strcmp(argv[1], "add") && argc == 3 && !parse_u32(argv[2], &v)) {
		rc = cmd_add(fd, v);
	} else if (!strcmp(argv[1], "sub") && argc == 3 && !parse_u32(argv[2], &v)) {
		rc = cmd_sub(fd, v);
	} else if (!strcmp(argv[1], "str") && argc == 3) {
		rc = cmd_str(fd, argv[2]);
	} else {
		print_help(argv[0]);
		rc = 1;
	}

	close(fd);
	return rc < 0 ? 1 : rc;
}
