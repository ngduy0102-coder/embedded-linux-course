#!/bin/sh

echo "===== TASK 2: COUNTER MISC DEVICE TEST ====="

echo "[1] Load driver"
insmod /lib/modules/5.15.0/counter_driver.ko
echo

echo "[2] Check device"
ls -l /dev/counter
echo

echo "[3] Initial /proc statistics"
cat /proc/counter_info
echo

echo "[4] READ test"
echo "First read:"
cat /dev/counter
echo "Second read:"
cat /dev/counter
echo

echo "[5] WRITE test"
echo 10 > /dev/counter
cat /dev/counter
echo

echo "[6] RESET test"
echo 0 > /dev/counter
cat /dev/counter
echo

echo "[7] Invalid input test"
echo abc > /dev/counter
echo -5 > /dev/counter
echo

echo "[8] Final statistics"
cat /proc/counter_info
echo

echo "[9] Unload driver"
rmmod counter_driver
ls -l /dev/counter 2>&1
cat /proc/counter_info 2>&1

echo
echo "===== TEST COMPLETE ====="
