#!/bin/sh

echo "===== TASK 2: COUNTER MISC DEVICE TEST ====="

echo
echo "[1] Load driver"
insmod /lib/modules/5.15.0/counter_driver.ko || exit 1

echo
echo "[2] Check device"
ls -l /dev/counter

echo
echo "[3] Initial statistics"
cat /proc/counter_info

echo
echo "[4] READ test"
echo "First read:"
cat /dev/counter
echo "Second read:"
cat /dev/counter

echo
echo "[5] WRITE test: set counter to 10"
echo 10 > /dev/counter || exit 1
echo "Statistics immediately after write:"
cat /proc/counter_info
echo "Read after write:"
cat /dev/counter

echo
echo "[6] RESET test: set counter to 0"
echo 0 > /dev/counter || exit 1
echo "Statistics immediately after reset:"
cat /proc/counter_info
echo "Read after reset:"
cat /dev/counter

echo
echo "[7] Invalid input tests"
if echo abc > /dev/counter; then
    echo "[FAIL] Nonnumeric input accepted"
else
    echo "[PASS] Nonnumeric input rejected"
fi

if echo -5 > /dev/counter; then
    echo "[FAIL] Negative input accepted"
else
    echo "[PASS] Negative input rejected"
fi

echo
echo "[8] Final statistics"
cat /proc/counter_info

echo
echo "[9] Driver kernel messages"
dmesg | grep 'counter_driver'

echo
echo "[10] Unload driver"
rmmod counter_driver || exit 1

echo "[PASS] Driver unloaded"

echo
echo "===== ALL TASK 2 TESTS COMPLETED ====="
