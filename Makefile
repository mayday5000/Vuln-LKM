# Out-of-tree LKM + CLI. Generated files land in build/.
KDIR  ?= /lib/modules/$(shell uname -r)/build
BUILD ?= $(CURDIR)/build

.PHONY: all module user clean

all: module user

$(BUILD):
	mkdir -p $(BUILD)

# kbuild needs a Makefile next to the .c; keep sources in the repo root.
module: $(BUILD)
	printf '%s\n' 'obj-m += vuln_lkm.o' 'ccflags-y += -g' > $(BUILD)/Makefile
	ln -sfn $(CURDIR)/vuln_lkm.c $(BUILD)/vuln_lkm.c
	ln -sfn $(CURDIR)/vuln_lkm.h $(BUILD)/vuln_lkm.h
	$(MAKE) -C $(KDIR) M=$(BUILD) modules
	@echo "module: $(BUILD)/vuln_lkm.ko"

# Static first so the busybox initramfs can run the CLI without libc.
user: $(BUILD)/vuln_lkm_cli

$(BUILD)/vuln_lkm_cli: vuln_lkm_cli.c vuln_lkm.h | $(BUILD)
	gcc -Wall -O2 -static -o $@ vuln_lkm_cli.c 2>/dev/null \
		|| gcc -Wall -O2 -o $@ vuln_lkm_cli.c
	@echo "cli: $@"

clean:
	@if [ -d $(BUILD) ]; then $(MAKE) -C $(KDIR) M=$(BUILD) clean || true; fi
	rm -rf $(BUILD)
