obj-m += mini_vuln.o
KDIR ?= /lib/modules/$(shell uname -r)/build

.PHONY: all module user clean

all: module user

module:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

user: mini_vuln_cli

mini_vuln_cli: mini_vuln_cli.c mini_vuln.h
	gcc -Wall -O2 -o $@ mini_vuln_cli.c

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	rm -f mini_vuln_cli int_bounds_cli
