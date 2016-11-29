#!/bin/bash
mount /dev/sdb1 /mnt > /dev/null
if [ -n "$1" ]; then
    echo "[+] mount usb succeed!!"
    cp $1 /mnt
    if [ $? = 0 ]; then
        echo "[+] copy file to usb finished!!"
    else
        echo "[-] copy file to usb falied!!"
        exit 1
    fi
else
    echo "[-] mount usb failed!!"
    exit 2
fi
umount /dev/sdb1 > /dev/null
if [ $? = 0 ]; then
    echo "[+] umount finished!!"
else
    echo "[-] umount falied!!"
    exit 3
fi
