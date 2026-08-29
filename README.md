# mini_vuln

Educational Linux miscdevice lab. Integer wrap (`ADD`/`SUB` on a `u32`) plus a string copy into a 32-byte kernel buffer with no length check vs that buffer. Not an exploit kit.

GitHub repo slug is still `int_bounds_driver`; the module and `/dev` node are `mini_vuln`.

## Build

```sh
make
chmod +x load.sh unload.sh
./load.sh
./mini_vuln_cli
```

Device node: `/dev/mini_vuln`.

## ioctls

| cmd | what |
|-----|------|
| GET / SET | read or store `g_val` |
| ADD / SUB | wrap on overflow / underflow |
| STR | copy `len` bytes into a 32-byte kernel stack buffer |

CLI option 5 prints the string and the base64 of the whole ioctl payload. Kernel buffer is 32 bytes; a longer string overflows it.
