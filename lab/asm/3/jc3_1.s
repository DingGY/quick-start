.section    .data
value16:
    .int 0x19
value10:
    .int 25
value8:
    .int 031
value2:
    .int 0b11001
.section .text
.globl  _start
_start:
    movl value16, %eax
    movl value10, %eax
    movl value8, %eax
    movl value2, %eax
    nop
