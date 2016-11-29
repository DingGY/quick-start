#!/bin/sh
if [ ! -n "$1" ]; then
    echo '[-] input file or -p to get or put!!'
    exit 2
fi
if [ "$1" = "-p" ]; then
    tftp -p -r "$2" -l "$2" 192.168.1.50 
    if [ $? = 0 ]; then
        echo "[+] $2 transform succeed!!"

    else
        echo "[-] $2 transform failed!!"
        exit 1
    fi
else
    tftp -g -l "$1" -r "$1" 192.168.1.50 && \
    if [ $? = 0 ]; then
        echo "[+] $1  download succeed!!"
    else
        echo "[-] $1 download failed!!"i
        exit 1
    fi
fi

