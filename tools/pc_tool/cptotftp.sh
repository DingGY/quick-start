#!/bin/bash

UPFLAG=0
FILEFLAG=0
ifconfig eth0 up && \
ifconfig eth0 192.168.1.50 netmask 255.255.255.0 && \
if [ $? = 0 ]; then
    UPFLAG=1
    if [ ! -n "$1" ]; then
        FILEFLAG=0
    else 
        FILEFLAG=1
        FILENAME=`echo "$1" | egrep -o "[^/]*$"`
        cp $1 /home/dinggy/Desktop/minilinux/tftpswap/ && \
        chmod 777 /home/dinggy/Desktop/minilinux/tftpswap/$FILENAME
    fi
fi
sleep 4
ifconfig eth0 down && \
sleep 1
if [ $UPFLAG = 1 ]; then
    echo "[+] eth0 up succeed!!"
else
    echo "[-] eth0 up failled!!"
fi
if [ $FILEFLAG = 0 ]; then
    echo "[-] input the file name"
else
    echo "[+] copy $FILENAME succeed!!"
fi

