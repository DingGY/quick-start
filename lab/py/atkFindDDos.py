#!/usr/bin/python
import socket
import pcap
import dpkt




def printPcap(pcap):
    for ts,buf in pcap:
        try:
            eth = dpkt.ethernet.Ethernet(buf)
            ip = eth.data
            tcp = ip.data
            src = socket.inet_ntoa(ip.src)
            http = dpkt.http.Request(tcp.data)
            if http.method == 'GET':
                uri = http.uri.lower()
                if ".rar" in uri:
                    print '[!] ' + src + ' Download: ' + uri

        except:
            #print 'analyse failed..'
            pass



def main():
    p = pcap.pcap()
    #p.setfilter('tcp port 80')
    printPcap(p)




__main__ = '__main__'
if __main__ == '__main__':
    main()
