#!/bin/sh
set -e
make module
sudo rmmod mini_vuln 2>/dev/null || true
sudo rmmod int_bounds 2>/dev/null || true
sudo insmod ./mini_vuln.ko
sudo chmod 666 /dev/mini_vuln 2>/dev/null || true
echo "loaded /dev/mini_vuln"
ls -l /dev/mini_vuln
