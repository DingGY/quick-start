#include <stdlib.h>
int main()
{
	__asm(
		"
		mov $0, %%edi
	 	mov $0xe7, %%eax
		syscall
		"
	);
}
