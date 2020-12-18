#!/bin/python3
from scapy.all import *
from scapy.contrib.igmp import *
import time
import threading
igmp_ctrl = Ether(src='90:e6:ba:5d:4f:dd',dst='01:00:5e:01:01:01')/IP(src='192.168.2.3',dst='227.1.1.1')/IGMP(type=0x16,gaddr='227.1.1.1')
igmp_ctrl_wl = Ether(src='00:1e:65:e7:df:34',dst='01:00:5e:01:01:01')/IP(src='192.168.2.2',dst='227.1.1.1')/IGMP(type=0x16, gaddr='227.1.1.1')
igmp_data_wl = Ether(src='00:1e:65:e7:df:34',dst='01:00:5e:01:01:01')/IP(src='192.168.2.2',dst='227.1.1.1')/UDP()
igmp_data = Ether(src='90:e6:ba:5d:4f:dd',dst='01:00:5e:01:01:01')/IP(src='192.168.1.1',dst='227.1.1.1')/UDP()
igmp_data_upnp = Ether(src='90:e6:ba:5d:4f:dd',dst='01:00:5e:ff:ff:fa')/IP(src='192.168.1.1',dst='239.255.255.250')/UDP()
uncast_vlandata = Ether(src='90:e6:ba:5d:4f:dd',dst='08:57:00:ee:82:65')/Dot1Q(vlan=45)/IP(src='192.168.1.1',dst='192.168.2.100')/UDP()
uncast_data = Ether(src='90:e6:ba:5d:4f:dd',dst='08:57:00:ee:82:65')/IP(src='192.168.1.1',dst='192.168.2.100')/UDP()
def main_thread():
    while True:
        sendp(igmp_data, iface='enp2s0')
        time.sleep(1)
def main():
    t = []
    for i in range(2):
        t.append(threading.Thread(target=main_thread))
        t[i].setDaemon(True)
        t[i].start()
    for p in t:
        p.join()




if 1: 
    main()
else:
    main_thread()
