#!/bin/python3
from scapy.all import *
from scapy.contrib.igmp import *
import time
igmp_ctrl = Ether(src='90:e6:ba:5d:4f:dd',dst='01:00:5e:01:01:01')/Dot1Q(vlan=66)/IP(src='192.168.1.1',dst='227.1.1.1')/IGMP(type=0x16)
igmp_data = Ether(src='90:e6:ba:5d:4f:dd',dst='01:00:5e:01:01:07')/IP(src='192.168.1.1',dst='227.1.1.7')/UDP()
uncast_data = Ether(src='90:e6:ba:5d:4f:dd',dst='08:57:00:ee:82:65')/Dot1Q(vlan=45)/IP(src='192.168.1.1',dst='192.168.2.100')/UDP()
while True:
    sendp(igmp_data, iface='enp2s0')
    #time.sleep(1)
