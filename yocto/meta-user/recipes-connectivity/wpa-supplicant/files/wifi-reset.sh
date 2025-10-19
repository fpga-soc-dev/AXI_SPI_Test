#!/bin/sh

echo "Resetting USB device 1-1..."
echo 0 > /sys/bus/usb/devices/1-1/authorized
sleep 1
echo 1 > /sys/bus/usb/devices/1-1/authorized
sleep 1

echo "Waiting for interface wlu1 to appear..."


COUNT=0
while [ $COUNT -lt 10 ]; do
    if [ -d "/sys/class/net/wlu1" ]; then
        echo "Interface wlu1 found. Success."
        exec /usr/sbin/wpa_supplicant -D nl80211 -u -i wlu1 -c /etc/wpa_supplicant.conf
        exit 0
    fi
    sleep 1
    COUNT=$((COUNT+1))
done

echo "Error: Interface wlu1 never appeared."
exit 1 # Fail
