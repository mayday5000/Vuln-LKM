# Install, run, QEMU, GDB

## Host crash vs guest crash

`vuln_lkm` is an LKM. `insmod` inserts it into **whichever kernel is running in that machine**.

| how you load | what can die | snapshot? |
|---|---|---|
| `HOST=1 ./load.sh` (insmod here) | this Ubuntu (especially `str` longer than 32 bytes) | yes, or use a disposable VM |
| `./load.sh` / `./install.sh` (QEMU) | only the QEMU guest | no |

ADD/SUB wrap a `u32` and almost never panic. STR `memcpy` into `kbuf[32]` can oops.

QEMU does **not** debug the live host kernel. It boots `build/bzImage`, which is a **copy** of `/boot/vmlinuz-$(uname -r)`, with a busybox initramfs that already contains `vuln_lkm.ko` and the CLI. Same vermagic, separate VM. `-no-reboot` so a guest panic just stops QEMU.

## install.sh

```sh
chmod +x install.sh load.sh unload.sh scripts/*.sh
./install.sh
```

That runs `scripts/install_qemu_gdb.sh` (QEMU, GDB, `linux-headers-$(uname -r)`, `busybox-static`, cpio), `make all` into `build/`, `scripts/build_initramfs.sh`, then `load.sh` (QEMU).

## Why the CLI was missing

`make module` only builds the `.ko`. `load.sh` used to call only that. `make` / `make all` now builds both into `build/`:

- `build/vuln_lkm.ko`
- `build/vuln_lkm_cli`

## Host path (dangerous)

```sh
make
HOST=1 ./load.sh
./build/vuln_lkm_cli get
./unload.sh
```

## QEMU path (default)

```sh
./scripts/build_initramfs.sh   # if you already have packages
./load.sh                      # or ./scripts/run_qemu.sh
```

Guest:

```
/vuln_lkm_cli get
/vuln_lkm_cli str hello
```

Quit QEMU: `Ctrl-A` then `X`.

## GDB

Other terminal while QEMU is up:

```sh
./scripts/debug_gdb.sh
```

Symbols: `build/vuln_lkm.ko`. After the guest `insmod` (initramfs does it for you):

```
guest$ cat /sys/module/vuln_lkm/sections/.text
(gdb) add-symbol-file build/vuln_lkm.ko 0xTHAT_ADDRESS
```

Breakpoints: `vuln_lkm_ioctl`, `vuln_lkm_add`, `vuln_lkm_sub`, `vuln_lkm_str`.

## Common failures

| symptom | likely cause |
|---|---|
| no `vuln_lkm_cli` | ran `make module` only; run `make` / `make all` |
| `file_operations` incomplete type | old tree without `#include <linux/fs.h>`; `git pull` |
| `make` / missing headers | `linux-headers-$(uname -r)` |
| QEMU: no bzImage | `/boot/vmlinuz-$(uname -r)` not copied; rerun `build_initramfs.sh` (may need sudo) |
| guest insmod Invalid module format | `.ko` not built for this `uname -r` |
| host oops on `str` | you used `HOST=1`; use QEMU |

See [vulnerabilities.md](vulnerabilities.md) and [cli.md](cli.md).
