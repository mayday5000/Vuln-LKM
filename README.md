# Vuln-LKM

Educational **Linux Loadable Kernel Module** (LKM). It is a `.ko` you `insmod` into a running kernel. It is **not** a built-in driver compiled into `vmlinux`, and it is **not** a PCI/platform/bus driver. After load it registers a misc **character device** `/dev/vuln_lkm`. Userspace talks to it only through `ioctl`.

An LKM is linked against the kernel it will join (`make -C /lib/modules/$(uname -r)/build M=$PWD`). `module_init` runs at `insmod`; `module_exit` at `rmmod`. This one uses the `miscdevice` API (`misc_register`) so you do not write a full `cdev` + `class_create` setup.

Intentional bugs (lab only, not an exploit kit):

1. **Integer overflow** — `vuln_lkm_add`: `g_val = g_val + n` on a `u32` with no saturation check.
2. **Integer underflow** — `vuln_lkm_sub`: `g_val = g_val - n` with no underflow check.
3. **String buffer overflow** — `vuln_lkm_str`: `memcpy` into a 32-byte stack buffer using a user `len` (capped only at 256, the ioctl struct, not at 32).

Deep paths, GDB notes, and `dmesg` what to look for: [Docs/vulnerabilities.md](Docs/vulnerabilities.md).

## PDFs

Black header, green footer, yellow syntax-colored source. Diagrams in the driver manual.

| file | pages | contents |
|------|-------|----------|
| [Docs/vuln_lkm.pdf](Docs/vuln_lkm.pdf) | 20 | How the LKM works: every function and its purpose, ABI, figures, kernel.org links |
| [Docs/vuln_lkm_cli.pdf](Docs/vuln_lkm_cli.pdf) | 11 | How the CLI works (ABI, argv/interactive, base64 dumps, full source) |
| [Docs/vuln_lkm_run_debug.pdf](Docs/vuln_lkm_run_debug.pdf) | 43 | Install, QEMU/GDB sessions, ioctl/IRQL/char-device theory, overflows, appendices |

## Layout

| file | role |
|------|------|
| `vuln_lkm.c` | LKM source (ioctl switch + three vuln functions) |
| `vuln_lkm.h` | ioctl numbers and structs, shared with the CLI |
| `vuln_lkm_cli.c` | userspace tester |
| `Makefile` | kbuild `.ko` + `gcc` CLI |
| `load.sh` / `unload.sh` | insmod / rmmod |
| `scripts/install_qemu_gdb.sh` | install QEMU + GDB |
| `scripts/run_qemu.sh` | boot guest with GDB stub `:1234` |
| `scripts/debug_gdb.sh` | attach GDB, breakpoints per vuln |
| `Docs/` | markdown + the three PDFs above |

## Build and run (host kernel)

```sh
make
chmod +x load.sh unload.sh scripts/*.sh
./load.sh
./vuln_lkm_cli --help
./vuln_lkm_cli get
```

## CLI help

```
./vuln_lkm_cli -h
```

```
Usage:
  ./vuln_lkm_cli                 interactive menu
  ./vuln_lkm_cli get
  ./vuln_lkm_cli set <n>
  ./vuln_lkm_cli add <n>
  ./vuln_lkm_cli sub <n>
  ./vuln_lkm_cli str <string>
```

Every ioctl prints the cmd hex, the number (dec+hex) or string, and base64 of the payload bytes.

## CLI examples

```sh
./vuln_lkm_cli set 10
./vuln_lkm_cli sub 11          # underflow → 4294967295
./vuln_lkm_cli get

./vuln_lkm_cli set 0xfffffffe
./vuln_lkm_cli add 3           # overflow → 1

./vuln_lkm_cli str hello
./vuln_lkm_cli str AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA   # 40 > 32
```

More: [Docs/cli.md](Docs/cli.md).

## QEMU + GDB

```sh
sudo ./scripts/install_qemu_gdb.sh
KERNEL=/path/to/bzImage ./scripts/run_qemu.sh   # terminal 1
./scripts/debug_gdb.sh                          # terminal 2
```

Breakpoints: `vuln_lkm_ioctl`, `vuln_lkm_add`, `vuln_lkm_sub`, `vuln_lkm_str`. Full walkthrough: [Docs/install-and-run.md](Docs/install-and-run.md) and [Docs/vuln_lkm_run_debug.pdf](Docs/vuln_lkm_run_debug.pdf).

Run STR tests in QEMU, not on a host you care about.
