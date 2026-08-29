# Install, run, QEMU, GDB

## What you are running

`vuln_lkm` is a **loadable kernel module** (LKM). You compile a `.ko` against the kernel headers of the kernel that will load it, then `insmod` it. It is not built into `vmlinux`. Once loaded it registers a misc character device `/dev/vuln_lkm`. Userspace talks to it with `ioctl`.

Host path (module in *this* running kernel):

```sh
make
chmod +x load.sh unload.sh scripts/*.sh
./load.sh
./vuln_lkm_cli --help
./vuln_lkm_cli get
```

Needs `build-essential` and `linux-headers-$(uname -r)`.

`load.sh` `insmod`s `vuln_lkm.ko` and ensures `/dev/vuln_lkm` is world-accessible. `unload.sh` `rmmod`s it.

## QEMU + GDB packages

```sh
sudo ./scripts/install_qemu_gdb.sh
```

Installs `qemu-system-x86_64` and `gdb` (apt/dnf/pacman). Also tries kernel headers / build tools so you can compile the LKM.

## QEMU runner

`scripts/run_qemu.sh` starts QEMU with a GDB stub on TCP port 1234 (`-gdb tcp::1234`), serial console, `nokaslr`, and a 9p share of this repo (mount tag `modshare`).

You must supply a kernel image. The script does not download one.

```sh
export KERNEL=/path/to/bzImage
export INITRD=/path/to/initramfs.cpio.gz   # optional
./scripts/run_qemu.sh
```

Use a debug-friendly `bzImage` (symbols, `nokaslr`). Build your own from kernel.org or reuse a distro debug kernel. Put `bzImage` in the repo root to use the default path.

In the guest, after you have a rootfs that can `insmod`:

```sh
mkdir -p /mod
mount -t 9p -o trans=virtio modshare /mod
insmod /mod/vuln_lkm.ko
ls -l /dev/vuln_lkm
```

If 9p is not in your initramfs, copy `vuln_lkm.ko` into the initrd instead.

## GDB script

In another terminal, while QEMU is up:

```sh
./scripts/debug_gdb.sh
```

That runs `gdb -x scripts/vuln_lkm.gdb`, which does `target remote localhost:1234` and sets:

| breakpoint | vulnerability |
|---|---|
| `vuln_lkm_ioctl` | every ioctl hits the dispatcher first |
| `vuln_lkm_add` | integer overflow (`g_val + n` wrap) |
| `vuln_lkm_sub` | integer underflow (`g_val - n` wrap) |
| `vuln_lkm_str` | string copy into 32-byte `kbuf` |

Module symbols are often pending until the `.ko` is loaded. After `insmod` in the guest:

```
guest$ cat /sys/module/vuln_lkm/sections/.text
(gdb) add-symbol-file vuln_lkm.ko 0xTHAT_ADDRESS
```

Then the four breakpoints can bind. `continue`, trigger the ioctl from the guest CLI, and GDB stops in the matching function.

Inspect:

- ADD/SUB: `p g_val`, `p n`, `p before` (step across the add/sub)
- STR: `p req.len`, `p/x req.data[0]@32`, `p &kbuf`, `p sizeof(kbuf)`

## Common failures

| symptom | likely cause |
|---|---|
| `make` fails on `M=` | missing `linux-headers-$(uname -r)` |
| `open /dev/vuln_lkm` | module not loaded, or udev node not created yet |
| QEMU exits immediately | `KERNEL` path wrong |
| GDB `target remote` refuses | QEMU not running, or port not 1234 |
| breakpoints pending forever | `.ko` not loaded, or no `add-symbol-file` |
| `nokaslr` ignored | cmdline not in `-append`, or kernel built without support |

See also [vulnerabilities.md](vulnerabilities.md) and [cli.md](cli.md).
