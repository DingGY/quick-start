#!/usr/bin/python
import geoip2.database
import sys
import pcap
import socket
import dpkt




def printPcap(pcap,loc):
    for ts,buf in pcap:
        eth = dpkt.ethernet.Ethernet(buf)
        ip = eth.data
        src = socket.inet_ntoa(ip.src)
        dst = socket.inet_ntoa(ip.dst)
        print '[+] Src:' + src + ' --> Dst:' + dst
        try:
            srcCity = loc.city(src).city.names['zh-CN']
        except:
            srcCity =  'unknow!!'
        try:
            dstCity = loc.city(dst).city.names['zh-CN']
        except:
            dstCity = 'unknow!!'
        print '[+] Src:' + srcCity + ' --> Dst:' + dstCity



def main():
    rd = geoip2.database.Reader("/home/dinggy/code/tool/GeoLite2-City_20170801/GeoLite2-City.mmdb")
    p = pcap.pcap()
    p.setfilter('tcp port 80')
    printPcap(p,rd)




__main__ = '__main__'
if __main__ == '__main__':
    main()
