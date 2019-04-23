#!/bin/bash
make clean
make
rmmod maclimit > /dev/null
insmod maclimit.ko
