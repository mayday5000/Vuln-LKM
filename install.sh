#!/bin/sh
# Install build/QEMU/busybox deps, build into build/, pack a guest initramfs,
# then load.sh (QEMU by default — does not insmod into this Ubuntu).
set -e
ROOT="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
cd "$ROOT"
chmod +x load.sh unload.sh install.sh scripts/*.sh 2>/dev/null || true

echo "=== packages (QEMU, GDB, headers, busybox-static) ==="
sudo "$ROOT/scripts/install_qemu_gdb.sh"

echo "=== build .ko + CLI into build/ ==="
make all

echo "=== busybox initramfs + host vmlinuz copy ==="
"$ROOT/scripts/build_initramfs.sh"

echo
echo "install done. artifacts:"
ls -l "$ROOT/build/vuln_lkm.ko" "$ROOT/build/vuln_lkm_cli" \
	"$ROOT/build/initramfs.cpio.gz" "$ROOT/build/bzImage" 2>/dev/null || true
echo
echo "Next: load.sh starts QEMU (guest). A panic there does not reboot this Ubuntu."
echo "Host insmod (can oops this machine): HOST=1 ./load.sh"
echo
exec "$ROOT/load.sh"
