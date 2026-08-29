#!/bin/sh
set -e
make module
sudo rmmod vuln_lkm 2>/dev/null || true
sudo rmmod mini_vuln 2>/dev/null || true
sudo rmmod int_bounds 2>/dev/null || true
sudo insmod ./vuln_lkm.ko
sudo chmod 666 /dev/vuln_lkm 2>/dev/null || true
echo "loaded /dev/vuln_lkm"
ls -l /dev/vuln_lkm
