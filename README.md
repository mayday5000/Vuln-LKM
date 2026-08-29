# int_bounds_driver

Tiny educational Linux miscdevice lab. One `u32` in the kernel. `ADD` and `SUB` wrap on purpose (integer overflow / underflow). Not an exploit kit.

## Build

```sh
make
chmod +x load.sh unload.sh
./load.sh
./int_bounds_cli
```

Needs kernel headers for the running kernel. Device node is `/dev/int_bounds`.

## ioctls

| cmd | what |
|-----|------|
| GET | read current `g_val` |
| SET | store a new `g_val` |
| ADD | `g_val += n` with wrap |
| SUB | `g_val -= n` with wrap |

Watch wrap in `dmesg` too. Example: SET 10, SUB 11 -> 4294967295. SET 0xfffffffe, ADD 3 -> 1.

The CLI prints the number (decimal + hex) and the base64 of the ioctl payload for every call.
