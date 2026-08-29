#!/bin/sh
# Boot a GUEST: host's vmlinuz copy + busybox initramfs, GDB stub :1234.
# This is not the running Ubuntu kernel. A guest oops does not panic the host.
set -e
ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
KERNEL="${KERNEL:-$ROOT/build/bzImage}"
INITRD="${INITRD:-$ROOT/build/initramfs.cpio.gz}"
GDB_PORT="${GDB_PORT:-1234}"
MEM="${MEM:-512M}"

if [ ! -f "$KERNEL" ] || [ ! -f "$INITRD" ]; then
	echo "missing $KERNEL or $INITRD" >&2
	echo "run: ./scripts/build_initramfs.sh   (or ./install.sh)" >&2
	exit 1
fi

echo "QEMU GUEST (not this Ubuntu)"
echo "  kernel=$KERNEL"
echo "  initrd=$INITRD"
echo "  gdb stub tcp::$GDB_PORT"
echo "  append: console=ttyS0 nokaslr rdinit=/init"
echo "  quit: Ctrl-A then X"

set -- qemu-system-x86_64 \
	-m "$MEM" \
	-cpu max \
	-smp 1 \
	-nographic \
	-no-reboot \
	-gdb "tcp::$GDB_PORT" \
	-kernel "$KERNEL" \
	-initrd "$INITRD" \
	-append "console=ttyS0 nokaslr earlyprintk=serial rdinit=/init" \
	-fsdev "local,id=mod,path=$ROOT,security_model=none" \
	-device "virtio-9p-pci,fsdev=mod,mount_tag=modshare"

if [ -e /dev/kvm ] && [ -r /dev/kvm ]; then
	set -- "$@" -enable-kvm
	echo "  kvm: yes"
else
	echo "  kvm: no (TCG, slower)"
fi

exec "$@"
