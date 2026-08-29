obj-m += vuln_lkm.o
KDIR ?= /lib/modules/$(shell uname -r)/build

.PHONY: all module user clean

all: module user

module:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

user: vuln_lkm_cli

vuln_lkm_cli: vuln_lkm_cli.c vuln_lkm.h
	gcc -Wall -O2 -o $@ vuln_lkm_cli.c

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	rm -f vuln_lkm_cli mini_vuln_cli int_bounds_cli
