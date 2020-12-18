#include <stdio.h>
void t1()
{
	char b[2];
	int r = 0;

	r = sprintf(b, "%-4s ======  %-4s", "123456789", "123456789");
	printf("%s--- %d\n", b,r);
}
int main()
{
	t1();
	printf("hello world!!\n");
}
