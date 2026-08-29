#!/bin/sh
# Boot a guest with the GDB stub on :1234 so scripts/debug_gdb.sh can attach.
#
# Required: a kernel image. Optional initramfs.
#   KERNEL=/path/to/bzImage INITRD=/path/to/initramfs.cpio.gz ./scripts/run_qemu.sh
#
# The repo is exported into the guest as 9p mount tag "modshare".
# Inside the guest (if your init mounts 9p):
#   mkdir -p /mod && mount -t 9p -o trans=virtio modshare /mod
#   insmod /mod/vuln_lkm.ko
#
# -s is shorthand for -gdb tcp::1234
# nokaslr keeps kernel text stable for breakpoints.
set -e

ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
KERNEL="${KERNEL:-$ROOT/bzImage}"
INITRD="${INITRD:-}"
GDB_PORT="${GDB_PORT:-1234}"
MEM="${MEM:-1024M}"

if [ ! -f "$KERNEL" ]; then
	echo "no kernel image at $KERNEL" >&2
	echo "set KERNEL=/path/to/bzImage (debug build, nokaslr recommended)" >&2
	echo "see Docs/install-and-run.md" >&2
	exit 1
fi

echo "QEMU kernel=$KERNEL"
echo "QEMU gdb stub tcp::$GDB_PORT  (then run ./scripts/debug_gdb.sh)"
echo "QEMU 9p share $ROOT -> mount tag modshare"
echo "append: console=ttyS0 nokaslr"

set -- qemu-system-x86_64 \
	-m "$MEM" \
	-cpu max \
	-smp 1 \
	-nographic \
	-gdb "tcp::$GDB_PORT" \
	-kernel "$KERNEL" \
	-append "console=ttyS0 nokaslr earlyprintk=serial" \
	-fsdev "local,id=mod,path=$ROOT,security_model=none" \
	-device "virtio-9p-pci,fsdev=mod,mount_tag=modshare"

if [ -n "$INITRD" ]; then
	if [ ! -f "$INITRD" ]; then
		echo "INITRD not found: $INITRD" >&2
		exit 1
	fi
	set -- "$@" -initrd "$INITRD"
	echo "QEMU initrd=$INITRD"
fi

exec "$@"
