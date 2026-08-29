# Vulnerable paths in vuln_lkm

Educational lab. These are intentional, obvious bugs. This document describes what the driver does, not how to turn them into privilege escalation.

Shared types live in `vuln_lkm.h`. Dispatch is `vuln_lkm_ioctl` in `vuln_lkm.c`. Userspace uses `vuln_lkm_cli`.

`g_val` is a static `u32` in the module, initial value `100`.

## Dispatcher

Path:

1. Userspace `ioctl(fd, cmd, &arg)` on `/dev/vuln_lkm`
2. VFS calls `unlocked_ioctl` → `vuln_lkm_ioctl`
3. `switch (cmd)` on the `_IOW/_IOR` numbers from `vuln_lkm.h` (magic `0x1B`)

GDB: `break vuln_lkm_ioctl`. `p/x cmd` tells you which case you are in.

GET and SET are the safe controls: GET `copy_to_user`s `g_val`; SET `copy_from_user`s a `u32` into `g_val`.

## 1. Integer overflow — `vuln_lkm_add`

Ioctl: `VULN_LKM_ADD` (`_IOW(0x1B, 3, struct vuln_lkm_int)`).

Concrete path:

```
CLI "add N" / menu 3
  → ioctl(fd, VULN_LKM_ADD, &req)   req.value = N
  → vuln_lkm_ioctl
  → copy_int_from_user → req.value
  → vuln_lkm_add(n)
  → g_val = g_val + n;              /* u32 modular wrap */
```

There is no `if (n > U32_MAX - g_val)` (or equivalent). Unsigned C wrap is defined: `0xfffffffe + 3 = 1`.

Observe: `dmesg` prints `ADD before + n = after (wrap=yes|no)`. Wrap is `g_val < before`.

GDB: `break vuln_lkm_add`. `p g_val`, `p n`, `si` over the add, `p g_val` again.

Example: SET `0xfffffffe`, ADD `3` → `g_val == 1`.

## 2. Integer underflow — `vuln_lkm_sub`

Ioctl: `VULN_LKM_SUB` (`_IOW(0x1B, 4, struct vuln_lkm_int)`).

Concrete path:

```
CLI "sub N" / menu 4
  → ioctl(fd, VULN_LKM_SUB, &req)
  → vuln_lkm_ioctl
  → copy_int_from_user
  → vuln_lkm_sub(n)
  → g_val = g_val - n;              /* u32 modular wrap */
```

There is no `if (n > g_val)`. `10 - 11 = 4294967295` (`0xffffffff`).

`dmesg` uses `wrap=yes` when `g_val > before`.

GDB: `break vuln_lkm_sub`. Same inspect pattern as ADD.

Example: SET `10`, SUB `11`.

## 3. String buffer overflow — `vuln_lkm_str`

Ioctl: `VULN_LKM_STR` (`_IOW(0x1B, 5, struct vuln_lkm_str)`).

The ioctl struct is:

```c
struct vuln_lkm_str {
    __u32 len;
    char data[256];   /* VULN_LKM_STR_MAX */
};
```

Kernel stack object:

```c
char kbuf[VULN_LKM_KBUF];  /* 32 bytes */
```

Concrete path:

```
CLI "str S" / menu 5
  → fill vuln_lkm_str { len = strlen(S), data = S }
  → ioctl(fd, VULN_LKM_STR, &arg)
  → vuln_lkm_ioctl
  → vuln_lkm_str(arg)
  → copy_from_user(&req, ..., sizeof(req))   /* whole struct, 256+4 bytes */
  → if (req.len > 256) req.len = 256;        /* cap vs ioctl blob only */
  → memcpy(kbuf, req.data, req.len);         /* NOT capped vs 32 */
```

The only cap is against the ioctl struct (`256`), not against `kbuf` (`32`). A `len` of 40 copies 40 bytes into a 32-byte stack array.

`dmesg` logs `STR len=... into kbuf[32]`.

GDB: `break vuln_lkm_str`. After `copy_from_user`: `p req.len`, `p req.data`. Before `memcpy`: `p sizeof(kbuf)` is 32. Step the `memcpy` and see `kbuf` / saved frame.

Example: `str hello` (len 5, fits). `str` with 40 `A`s (len 40, overflows `kbuf`).

This can panic the guest if you smash far enough. Use a VM (QEMU) for STR tests, not a machine you care about.

## What this is not

The CLI only issues the ioctls and prints payloads. There is no privilege-escalation payload in this repo. See the original HEVD-style practice driver for a larger set of bug classes; this LKM is only wrap + one string copy.
