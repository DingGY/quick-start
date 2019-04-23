maclimit.ko is a module that limit the number of internet on the lan side by arp
usage:
 
echo [num] [enable/disable] > /proc/net/netfilter/maclimit

num:alow 0~255 device to connect the internet if enable the maclimit
enable/disable: just...you know
