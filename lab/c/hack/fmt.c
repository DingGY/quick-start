#include <stdio.h>
int main(int argc, char *argv[])
{
	char tmp[4096];
	sprintf(tmp, "%.4000s\n", argv[1]);
	printf(tmp);
	printf("\n");
}
