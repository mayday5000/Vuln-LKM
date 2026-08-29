# Vuln-LKM

Educational **Linux Loadable Kernel Module** (LKM). After load it registers a misc **character device** `/dev/vuln_lkm`. Userspace talks to it only through `ioctl`.

Intentional bugs (lab only, not an exploit kit):

1. **Integer overflow** — `vuln_lkm_add`: `g_val = g_val + n` on a `u32` with no saturation check.
2. **Integer underflow** — `vuln_lkm_sub`: `g_val = g_val - n` with no underflow check.
3. **String buffer overflow** — `vuln_lkm_str`: `memcpy` into a 32-byte stack buffer using a user `len` (capped only at 256, not at 32).

## Will this crash my Ubuntu?

**If you `insmod` on the host (`HOST=1 ./load.sh`): yes, it can.** GET/SET/ADD/SUB usually just wrap a number and stay up. `str` with more than 32 bytes can oops or panic **this** kernel and you reboot or restore a snapshot. Do not do that on a machine you care about.

**Default `./load.sh` / `./install.sh` uses QEMU.** The guest boots a *copy* of your host's `vmlinuz` plus a busybox initramfs. That is the same kernel *version* (so `vuln_lkm.ko` matches), but it is **not** the already-running Ubuntu. A guest panic dies in QEMU (`-no-reboot`). The host stays up. No snapshot required for the QEMU path.

QEMU is **not** attaching to the live host kernel. It is a separate VM.

## Quick start (safe)

```sh
chmod +x install.sh load.sh unload.sh scripts/*.sh
./install.sh          # packages, build/, busybox initramfs, then QEMU
# in the guest:
/vuln_lkm_cli get
```

Artifacts: `build/vuln_lkm.ko`, `build/vuln_lkm_cli`, `build/initramfs.cpio.gz`, `build/bzImage`.

```sh
make                  # .ko + CLI into build/ (CLI was missing if you only ran make module)
./load.sh             # QEMU guest
HOST=1 ./load.sh      # host insmod — can panic this Ubuntu
./scripts/debug_gdb.sh
```

## PDFs

| file | pages | contents |
|------|-------|----------|
| [Docs/vuln_lkm.pdf](Docs/vuln_lkm.pdf) | 20 | How the LKM works |
| [Docs/vuln_lkm_cli.pdf](Docs/vuln_lkm_cli.pdf) | 11 | How the CLI works |
| [Docs/vuln_lkm_run_debug.pdf](Docs/vuln_lkm_run_debug.pdf) | 43 | Install, QEMU/GDB, ioctl/IRQL/overflows |

## Layout

| file | role |
|------|------|
| `vuln_lkm.c` / `vuln_lkm.h` | LKM + shared ioctl ABI |
| `vuln_lkm_cli.c` | userspace tester (binary: `build/vuln_lkm_cli`) |
| `Makefile` | kbuild + gcc; outputs under `build/` |
| `install.sh` | packages, build, initramfs, then `load.sh` |
| `load.sh` | QEMU by default; `HOST=1` insmod on this kernel |
| `scripts/build_initramfs.sh` | busybox rootfs + copy of `/boot/vmlinuz-$(uname -r)` |
| `scripts/run_qemu.sh` | guest, GDB stub `:1234` |
| `scripts/debug_gdb.sh` | attach GDB to the guest |
| `Docs/` | markdown + PDFs |

## CLI

```
./build/vuln_lkm_cli -h
./build/vuln_lkm_cli get
./build/vuln_lkm_cli set 10
./build/vuln_lkm_cli sub 11          # underflow → 4294967295
./build/vuln_lkm_cli set 0xfffffffe
./build/vuln_lkm_cli add 3           # overflow → 1
./build/vuln_lkm_cli str hello       # fits in 32
```

In QEMU the binary is `/vuln_lkm_cli`. Long `str` belongs there, not on the host.

More: [Docs/cli.md](Docs/cli.md), [Docs/install-and-run.md](Docs/install-and-run.md).
