/***********************************************************************
 *  name: board_event app
 *  Author: dingguangyu
 *  date: 2018.6.4
 *
 * HGUB963XX-34 dingguangyu add for customer dev 2018.6.4 
************************************************************************/

/** Includes. **/

#include <stdio.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include "board_event.h"


#define BOARD_EVENT_DEVFILE_PATH "/dev/board_event"


#define DEBUG
#ifdef DEBUG
#define BRD_BTN_DEBUG(fmt, arg...) printf(fmt, ##arg)
#else
#define BRD_BTN_DEBUG(fmt, arg...) 
#endif

/** Globals. **/
static int wifi_onoff_status = 0;
static void ltrim(char *s)
{
	char *p;
	p = s;
	while(*p == ' ' || *p == '\t' || *p == '\n')
		p++;
	strcpy(s,p);
}
static void rtrim(char *s)
{
	int i;
	i = strlen(s) - 1;
	while((s[i] == ' ' || s[i] == '\t' || s[i] == '\n') && i >= 0)
		i--;
	s[i+1] = '\0';
}
static void trim(char *s)
{
	ltrim(s);
	rtrim(s);
}








static int handle_dev_info(char *msg)
{
	
	BRD_BTN_DEBUG("handle_dev_info %s\n", msg);
	if(!strcmp(msg, WIFI_ONOFF_KEY))
	{
		BRD_BTN_DEBUG("handle_dev_info wifi: %s\n", msg);		
	}
	else
	{
		BRD_BTN_DEBUG("handle_dev_info unused msg: %s\n", msg);		
	}
	return 0;
}




/***************************************************************************
 * Function Name: main
 * Description  : Main program function.
 * Returns      : 0 - success, non-0 - error.
 ***************************************************************************/
int main(int argc, char **argv)
{
	struct pollfd btnfd;
	int conffd;
	int ret;
	int recvnum;	
	int enable_status = 1;
	char recv_buf[RECV_MAX_BUF_SIZE];
	

	btnfd.fd = open(BOARD_EVENT_DEVFILE_PATH,O_RDWR);
	if(btnfd.fd < 0)
	{
		BRD_BTN_DEBUG("board_event: open %s failed!\n",BOARD_BUTTON_DEVFILE_PATH);
		goto fail;
	}
	btnfd.events = POLLIN;
	while(1)
	{
		ret = poll(&btnfd,1,-1);
		if(ret == -1)
		{
			BRD_BTN_DEBUG("board_event: poll failed! ret: %d\n", ret);
			break;
		}
		if(btnfd.revents & POLLIN)
		{
			recvnum = read(btnfd.fd, recv_buf, RECV_MAX_BUF_SIZE);
			BRD_BTN_DEBUG("board_event: recved: data %s, recvnum %d\n", recv_buf, recvnum);
			handle_dev_info(recv_buf);
		}
	}
fail:
	BRD_BTN_DEBUG("board_event: exit for failed!!\n");
	close(btnfd.fd);
	return 0;
}
