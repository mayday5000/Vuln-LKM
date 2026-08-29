#!/bin/sh
set -e
make module
sudo rmmod int_bounds 2>/dev/null || true
sudo insmod ./int_bounds.ko
sudo chmod 666 /dev/int_bounds 2>/dev/null || true
echo "loaded /dev/int_bounds"
ls -l /dev/int_bounds
