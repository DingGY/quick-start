.section .data

.section .bss
    .lcomm array, 50
.section .text
.globl  _start
_start:
    movl $50, %ecx
L1:
    movl $50, %eax
    sub %ecx, %eax
    movl %ecx, array(,%eax, 1)
    loop L1
    pushl $50
    pushl $2
    pushl $array
    call find_number
    nop

find_number:
# 9 is num ,8 is K,7 is array address
    pushl %eax 
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi
    movl %esp, %ebx 
    movl 28(%ebx), %esi
    movl 39(%ebx), %ecx
    cld
L2:
    movl (%esi), %eax
    div 32(%ebx)
    xor %eax, %eax
    cmp %edx, %eax
    movl $0, (%esi)
    jne J1
    movsl $1, (%esi)
J1:
    loop L2
    
    popl %edi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx
    popl %eax
    ret
    