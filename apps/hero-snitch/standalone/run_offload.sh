#!/bin/sh

echo 1 > /proc/sys/kernel/printk
mount -t debugfs none /sys/kernel/debug ||:

insmod /lib/modules/5.16.9/extra/snitch.ko

./bringup snitch.bin file_to_offload.txt | tee -a run.log

rmmod snitch.ko