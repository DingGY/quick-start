.section    .data
 
.section    .text

.globl  _start
_start:
    movl $4, %eax
    movl $3, %ebx
    movl $2, %ecx
    movl $1, %edx
    addl %ebx, %eax
    subl %edx, %ecx
    subl %ecx, %eax
    nop
