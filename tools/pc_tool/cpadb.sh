#!/bin/bash
if [ "$1" == "" ]; then
    echo "[-] input file name."
    exit 1
fi
cp $1 /home/dinggy/Desktop/minilinux/adb/
if [ $? = 0 ]; then
    echo "[+] copy file to usb finished!!"
else
    echo "[-] copy file to usb falied!!"
    exit 1
fi

