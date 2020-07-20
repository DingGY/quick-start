#include "util.h"
int main()
{
	int listenfd,connfd;
	struct sockaddr_in servaddr;
	char buff[4096];


	if((listenfd = socket(AF_INET,SOCK_STREAM,0)) == -1){
		ERROR("create socket error: %s\n", strerror(errno));
		exit(0);
	}

	memset(servaddr, 0, sizeof(servaddr));	
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htol(INADDR_ANY);
}
