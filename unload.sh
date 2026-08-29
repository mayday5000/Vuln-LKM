#!/bin/sh
# rmmod if the module was loaded on the HOST. For QEMU, quit the guest (Ctrl-A X).
set -e
if lsmod 2>/dev/null | grep -q '^vuln_lkm'; then
	sudo rmmod vuln_lkm
	echo "unloaded vuln_lkm from HOST"
else
	echo "vuln_lkm is not loaded on this kernel."
	echo "If you are in QEMU, quit the guest: Ctrl-A then X."
fi
