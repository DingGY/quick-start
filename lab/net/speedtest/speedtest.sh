#!/bin/sh
insmod /lib/modules/speedtest.bin
while true
do
	DATA=`cat /proc/net/speedtest | grep "READY_3"`
	if [ "$DATA" != "" ];then
		echo ratelimit rx w Disable 64 > /proc/qdma_lan/ratelimit
		switchmgr stormctrl type 0
		sys memwl bfb58010 0000ffe0
		switchmgr vlan pvid 2 10
		switchmgr vlan pvid 3 10
		switchmgr vlan vid 10 1 10 12 0 1 0 0
		switchmgr vlan pvid 6 6
		switchmgr vlan vid 6 1 6 82 0 1 0 0
		switchmgr vlan pvid 4 4
		switchmgr vlan vid 4 1 4 80 0 1 0 0
		switchmgr vlan pvid 1 1
		switchmgr vlan vid 1 1 1 66 0 1 0 0
		echo "1 30 400 1700 0" > /proc/net/speedtest
	fi
	sleep 1
done
