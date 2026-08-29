#!/bin/sh
# Install QEMU (x86_64 system emulator) and GDB for debugging vuln_lkm.
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
		linux-headers-$(uname -r) || apt-get install -y qemu-system-x86 gdb build-essential
	;;
fedora|rhel|centos)
	dnf install -y qemu-system-x86 gdb gcc make kernel-devel || yum install -y qemu-system-x86 gdb gcc make
	;;
arch)
	pacman -Sy --noconfirm qemu-system-x86 gdb base-devel
	;;
*)
	echo "unknown distro; install qemu-system-x86_64 and gdb yourself" >&2
	exit 1
	;;
esac

echo
echo "installed:"
command -v qemu-system-x86_64
qemu-system-x86_64 --version | head -n1
command -v gdb
gdb --version | head -n1
echo "done."
