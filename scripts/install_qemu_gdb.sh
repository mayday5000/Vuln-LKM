#!/bin/sh
# Install QEMU, GDB, headers, busybox-static, cpio (initramfs).
set -e

if [ "$(id -u)" -ne 0 ]; then
	echo "re-run with sudo: sudo $0" >&2
	exit 1
fi

. /etc/os-release
echo "distro: ${ID:-unknown} ${VERSION_ID:-}"

case "${ID}" in
debian|ubuntu|linuxmint)
	export DEBIAN_FRONTEND=noninteractive
	apt-get update
	apt-get install -y --no-install-recommends \
		qemu-system-x86 \
		gdb \
		gdb-multiarch \
		build-essential \
		flex bison libncurses-dev libssl-dev libelf-dev \
		linux-headers-$(uname -r) \
		busybox-static \
		cpio gzip \
		libc6-dev || apt-get install -y qemu-system-x86 gdb build-essential busybox-static cpio
	;;
fedora|rhel|centos)
	dnf install -y qemu-system-x86 gdb gcc make kernel-devel busybox cpio gzip \
		|| yum install -y qemu-system-x86 gdb gcc make busybox cpio
	;;
arch)
	pacman -Sy --noconfirm qemu-system-x86 gdb base-devel busybox cpio gzip
	;;
*)
	echo "unknown distro; install qemu-system-x86_64, gdb, busybox, cpio" >&2
	exit 1
	;;
esac

echo
echo "installed:"
command -v qemu-system-x86_64
qemu-system-x86_64 --version | head -n1
command -v gdb
gdb --version | head -n1
command -v busybox || command -v busybox-static || true
busybox 2>/dev/null | head -n1 || true
echo "done."
