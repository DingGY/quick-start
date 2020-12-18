import struct
import sys

fi = open(sys.argv[1], 'rb')
fo = open(sys.argv[2], 'wb')

while True:
	c = fi.read(1)
	if c == b'':
		break
	a = struct.unpack('B', c)[0]
	a += 0x11
	fo.write(struct.pack('B', a))

fi.close()
fo.close()
