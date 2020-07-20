#!/bin/bash
if [ $1 = "g" ];then
	gdb test
	exit
fi

make clean
make
