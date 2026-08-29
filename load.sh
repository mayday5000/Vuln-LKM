#!/bin/sh
# Default: boot the lab in QEMU (busybox initramfs + a copy of the host vmlinuz).
# HOST=1 ./load.sh  — insmod into THIS Ubuntu. STR can oops/panic it.
set -e
ROOT="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
cd "$ROOT"
make all

if [ "${HOST:-0}" = 1 ]; then
	echo "WARNING: insmod on the HOST kernel ($ROOT/build/vuln_lkm.ko)." >&2
	echo "GET/SET/ADD/SUB usually just wrap a u32. STR with len>32 can oops this Ubuntu." >&2
	echo "Prefer ./load.sh (QEMU) unless this machine is disposable." >&2
	sudo rmmod vuln_lkm 2>/dev/null || true
	sudo rmmod mini_vuln 2>/dev/null || true
	sudo rmmod int_bounds 2>/dev/null || true
	sudo insmod "$ROOT/build/vuln_lkm.ko"
	sudo chmod 666 /dev/vuln_lkm 2>/dev/null || true
	echo "loaded /dev/vuln_lkm (HOST)"
	ls -l /dev/vuln_lkm
	echo "CLI: $ROOT/build/vuln_lkm_cli"
	exit 0
fi

"$ROOT/scripts/build_initramfs.sh"
echo "starting QEMU (guest).  Ctrl-A then X to quit.  GDB: ./scripts/debug_gdb.sh"
exec "$ROOT/scripts/run_qemu.sh"
