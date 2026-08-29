# GDB commands for vuln_lkm. Sourced by scripts/debug_gdb.sh
set pagination off
set confirm off

# Guest gdb stub (run_qemu.sh uses -gdb tcp::1234)
target remote localhost:1234

# Dispatcher + one function per vulnerability.
break vuln_lkm_ioctl
break vuln_lkm_add
break vuln_lkm_sub
break vuln_lkm_str

printf "\nBreakpoints for vuln_lkm:\n"
info breakpoints
printf "\nIf these pending breaks never resolve, the .ko is not loaded yet.\n"
printf "In the guest: insmod vuln_lkm.ko\n"
printf "Then add-symbol-file vuln_lkm.ko <addr>  where addr is:\n"
printf "  cat /sys/module/vuln_lkm/sections/.text\n\n"
