# CLI help and examples

Binary: `./vuln_lkm_cli` after `make user` (or `make`). Device must exist (`./load.sh`).

## Help

```
./vuln_lkm_cli --help
./vuln_lkm_cli -h
```

Prints usage, ioctl list, and the examples below. Interactive menu also has `h`.

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

Read the current kernel `g_val` (starts at 100 after a fresh insmod):

```
./vuln_lkm_cli get
```

Set, then underflow:

```
./vuln_lkm_cli set 10
./vuln_lkm_cli sub 11
./vuln_lkm_cli get
```

`get` should show `4294967295` (`0xffffffff`). `dmesg` should say `wrap=yes`.

Overflow:

```
./vuln_lkm_cli set 0xfffffffe
./vuln_lkm_cli add 3
./vuln_lkm_cli get
```

`get` should show `1`.

String that fits in 32 bytes:

```
./vuln_lkm_cli str hello
```

String longer than the kernel `kbuf` (40 bytes):

```
./vuln_lkm_cli str AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
```

Do that in QEMU. Watch `dmesg` for `STR len=40 into kbuf[32]`. In GDB, `break vuln_lkm_str`.

Interactive:

```
./vuln_lkm_cli
> 1
> 2
value to SET (dec or 0xhex): 10
> 4
value to SUB (dec or 0xhex): 11
> 5
string to send: hello
> q
```
