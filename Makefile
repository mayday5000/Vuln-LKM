obj-m += int_bounds.o
KDIR ?= /lib/modules/$(shell uname -r)/build

.PHONY: all module user clean

all: module user

module:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

user: int_bounds_cli

int_bounds_cli: int_bounds_cli.c int_bounds.h
	gcc -Wall -O2 -o $@ int_bounds_cli.c

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	rm -f int_bounds_cli
