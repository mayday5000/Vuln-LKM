#!/bin/sh
# Attach GDB to the QEMU stub started by scripts/run_qemu.sh.
# Breakpoints: vuln_lkm_ioctl (dispatch), vuln_lkm_add, vuln_lkm_sub, vuln_lkm_str.
set -e

ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
GDB="${GDB:-gdb}"
SYMBOLS="${SYMBOLS:-$ROOT/vuln_lkm.ko}"
GDBSCRIPT="$ROOT/scripts/vuln_lkm.gdb"

if ! command -v "$GDB" >/dev/null 2>&1; then
	echo "gdb not found; run sudo ./scripts/install_qemu_gdb.sh" >&2
	exit 1
fi

if [ ! -f "$GDBSCRIPT" ]; then
	echo "missing $GDBSCRIPT" >&2
	exit 1
fi

echo "connecting to QEMU gdb stub localhost:1234"
echo "breakpoints: vuln_lkm_ioctl, vuln_lkm_add, vuln_lkm_sub, vuln_lkm_str"
if [ -f "$SYMBOLS" ]; then
	echo "module file: $SYMBOLS (add-symbol-file after insmod; see Docs/install-and-run.md)"
fi

exec "$GDB" -q -x "$GDBSCRIPT" ${SYMBOLS:+"$SYMBOLS"}
