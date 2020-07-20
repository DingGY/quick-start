#!/usr/bin/python
import re
from scapy.all import *

pcapfile = '/home/dinggy/code/py/atkArp/arper.pcap'
pcap_file = 'arper'

def get_http_headers(http_payload):
    try:
        headers_raw = http_payload[:http_payload.index("\r\n\r\n") + 2]
        headers = dict(re.findall(r"(?P<name>.*?): (?P<value>.*?)\r\n", headers_raw))
        #print headers
    except Exception,e:
        return None
    if 'Content-Type' not in headers:
        return None
    return headers

def extract_image(headers, http_payload):
    image = None
    image_type = None
    try:
        if 'image' in headers['Content-Type']:
            image_type = headers['Content-Type'].split('/')[1]
            image = http_payload[http_payload.index('\r\n\r\n')+4:]
    except Exception,e:
        return None,None
    return image,image_type


def http_assembler(pfile):
    carved_count = 0

    pr = rdpcap(pfile)
    se = pr.sessions()
    for s in se:
        http_payload = ''
        '''get one http packet'''
        for p in se[s]:
            try:
                if p[TCP].dport == 80 or p[TCP].sport == 80:
                    http_payload += str(p[TCP].payload)
            except Exception,e:
                pass
        headers = get_http_headers(http_payload)
        if headers is None:
            continue
        image,image_type = extract_image(headers, http_payload)
        if image is not None and image_type is not None:
            file_name = "%s-id_%d.%s" % \
                    (pcap_file, carved_count, image_type)
            fd = open(file_name,"wb")
            fd.write(image)
            fd.close
            carved_count += 1
http_assembler(pcapfile)
