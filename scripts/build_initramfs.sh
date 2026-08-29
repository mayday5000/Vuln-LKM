#!/bin/sh
# Pack build/initramfs.cpio.gz: busybox + vuln_lkm.ko + vuln_lkm_cli.
# Copy this machine's vmlinuz to build/bzImage so QEMU boots the SAME kernel
# version the .ko was built against (a guest, not the running host).
set -e
ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build"
ROOTFS="$BUILD/rootfs"
KVER="$(uname -r)"

make -C "$ROOT" all

BB=""
for c in /bin/busybox /usr/bin/busybox; do
	if [ -x "$c" ]; then BB="$c"; break; fi
done
if [ -z "$BB" ]; then
	echo "busybox not found. sudo ./scripts/install_qemu_gdb.sh (installs busybox-static)" >&2
	exit 1
fi

rm -rf "$ROOTFS"
mkdir -p "$ROOTFS/bin" "$ROOTFS/sbin" "$ROOTFS/dev" "$ROOTFS/proc" \
	"$ROOTFS/sys" "$ROOTFS/tmp" "$ROOTFS/lib" "$ROOTFS/lib64" "$ROOTFS/usr/bin"

cp "$BB" "$ROOTFS/bin/busybox"
chmod 755 "$ROOTFS/bin/busybox"
# applets we need in the guest
for a in sh ash ls mount umount insmod rmmod chmod mknod mkdir ln echo cat \
	sleep setsid reboot poweroff dmesg uname lsmod; do
	ln -sf busybox "$ROOTFS/bin/$a"
done
ln -sf busybox "$ROOTFS/bin/cttyhack" 2>/dev/null || true

cp "$BUILD/vuln_lkm.ko" "$ROOTFS/vuln_lkm.ko"
cp "$BUILD/vuln_lkm_cli" "$ROOTFS/vuln_lkm_cli"
chmod 755 "$ROOTFS/vuln_lkm_cli"

# If the CLI is dynamically linked, pull in its loader + libs.
if command -v ldd >/dev/null 2>&1; then
	ldd "$BUILD/vuln_lkm_cli" 2>/dev/null | while read -r line; do
		# "lib.so => /path (addr)"  or  "/lib64/ld-linux-x86-64.so.2 (addr)"
		src=""
		case "$line" in
		*/*)
			src="$(printf '%s\n' "$line" | awk '{
				for (i=1;i<=NF;i++) if ($i ~ /^\//) { print $i; exit }
			}')"
			;;
		esac
		[ -n "$src" ] && [ -f "$src" ] || continue
		mkdir -p "$ROOTFS$(dirname "$src")"
		cp -a "$src" "$ROOTFS$src" 2>/dev/null || cp "$src" "$ROOTFS$src"
	done || true
fi

cat > "$ROOTFS/init" << 'INIT'
#!/bin/busybox sh
/bin/busybox mkdir -p /proc /sys /dev /tmp
/bin/busybox mount -t proc proc /proc
/bin/busybox mount -t sysfs sysfs /sys
/bin/busybox mount -t devtmpfs devtmpfs /dev 2>/dev/null || {
	/bin/busybox mknod /dev/console c 5 1
	/bin/busybox mknod /dev/ttyS0 c 4 64
	/bin/busybox mknod /dev/null c 1 3
}
echo "vuln_lkm busybox initramfs"
echo "this is a QEMU GUEST. a panic here does not reboot the Ubuntu host."
if [ -f /vuln_lkm.ko ]; then
	if /bin/busybox insmod /vuln_lkm.ko; then
		echo "insmod /vuln_lkm.ko ok"
	else
		echo "insmod failed (vermagic mismatch?)"
	fi
fi
/bin/busybox chmod 666 /dev/vuln_lkm 2>/dev/null || true
echo "CLI: /vuln_lkm_cli get|set|add|sub|str"
echo "STR with a long string can panic THIS guest only."
if /bin/busybox --list 2>/dev/null | /bin/busybox grep -q cttyhack; then
	exec /bin/busybox setsid /bin/busybox cttyhack /bin/busybox sh
fi
exec /bin/busybox sh
INIT
chmod 755 "$ROOTFS/init"

( cd "$ROOTFS" && find . | cpio -o -H newc --quiet ) | gzip -9 > "$BUILD/initramfs.cpio.gz"
echo "initramfs: $BUILD/initramfs.cpio.gz"

# Host vmlinuz -> build/bzImage (readable copy for QEMU -kernel)
IMG=""
for c in "/boot/vmlinuz-$KVER" /boot/vmlinuz; do
	if [ -e "$c" ]; then IMG="$c"; break; fi
done
if [ -z "$IMG" ]; then
	echo "no /boot/vmlinuz-$KVER — set KERNEL= when running QEMU" >&2
	exit 0
fi
if [ -r "$IMG" ]; then
	cp "$IMG" "$BUILD/bzImage"
else
	echo "copying $IMG (needs sudo, file is not world-readable)"
	sudo cp "$IMG" "$BUILD/bzImage"
	sudo chmod a+r "$BUILD/bzImage"
fi
echo "kernel:  $BUILD/bzImage  (copy of $IMG, version $KVER)"
echo "QEMU boots this IMAGE in a VM. It is not the already-running host kernel."
