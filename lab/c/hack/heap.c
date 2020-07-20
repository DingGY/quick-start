#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void shellcode()
{
	printf("we made it!!!");
}
int main(int argc, char *argv[])
{
	char *first,*sceond;
	char buffer[4096]; 
	printf("start..\n");
 	getchar();
 	first = malloc(1024);
 	printf("first malloc..\n");
 	getchar();
 	sceond = malloc(512);
 	printf("sceond malloc..\n");
 	getchar();
 	strcpy(first, argv[1]);
 	printf("strcpy to first..\n");
 	getchar();
 	free(first);
 	printf("first free..\n"); 
 	getchar();
 	free(sceond);
 	printf("sceond free..\n"); 
 	getchar();
	return 0;
}
