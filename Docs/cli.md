# CLI help and examples

Binary after `make` / `make all`: **`./build/vuln_lkm_cli`** (not the repo root; `make module` only builds the `.ko`).

Device:

- QEMU (default `./load.sh` / `./install.sh`): initramfs already `insmod`s; guest binary is **`/vuln_lkm_cli`**.
- Host (dangerous): `HOST=1 ./load.sh` then `./build/vuln_lkm_cli`. STR longer than 32 bytes can oops this Ubuntu.

## Help

```
./build/vuln_lkm_cli --help
./build/vuln_lkm_cli -h
```

In the guest: `/vuln_lkm_cli -h`. Interactive menu also has `h`.

## Help text (what you should see)

```
vuln_lkm_cli — userspace tester for the vuln_lkm educational LKM

Usage:
  ./vuln_lkm_cli                 interactive menu
  ./vuln_lkm_cli -h|--help       this help
  ./vuln_lkm_cli get             VULN_LKM_GET
  ./vuln_lkm_cli set <n>         VULN_LKM_SET
  ./vuln_lkm_cli add <n>         VULN_LKM_ADD  (u32 wrap)
  ./vuln_lkm_cli sub <n>         VULN_LKM_SUB  (u32 wrap)
  ./vuln_lkm_cli str <string>    VULN_LKM_STR  (copy into 32-byte kbuf)

<n> is decimal or 0xhex.
Every ioctl prints cmd hex, the number or string, and base64 of the payload.
Device: /dev/vuln_lkm
```

Each call prints:

- `ioctl cmd:` hex of the `_IOW/_IOR` number
- `number sent:` decimal and `0x........`  **or** `string sent:` plus `len sent:`
- `payload b64:` base64 of the bytes actually passed to `ioctl` (`struct vuln_lkm_int` is 4 bytes; `struct vuln_lkm_str` is 4+256)

## Examples

Use `/vuln_lkm_cli` inside QEMU, or `./build/vuln_lkm_cli` on the host.

Read the current kernel `g_val` (starts at 100 after a fresh insmod):

```
./build/vuln_lkm_cli get
```

Set, then underflow:

```
./build/vuln_lkm_cli set 10
./build/vuln_lkm_cli sub 11
./build/vuln_lkm_cli get
```

`get` should show `4294967295` (`0xffffffff`). `dmesg` should say `wrap=yes`.

Overflow:

```
./build/vuln_lkm_cli set 0xfffffffe
./build/vuln_lkm_cli add 3
./build/vuln_lkm_cli get
```

`get` should show `1`.

String that fits in 32 bytes:

```
./build/vuln_lkm_cli str hello
```

String longer than the kernel `kbuf` (40 bytes) — **QEMU only**:

```
/vuln_lkm_cli str AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
```

Watch guest dmesg for `STR len=40 into kbuf[32]`. In GDB, `break vuln_lkm_str`.

Interactive:

```
./build/vuln_lkm_cli
> 1
> 2
value to SET (dec or 0xhex): 10
> 4
value to SUB (dec or 0xhex): 11
> 5
string to send: hello
> q
```
