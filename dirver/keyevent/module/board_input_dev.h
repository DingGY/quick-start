#ifndef _BOARD_INPUT_DEV_H_
#define _BOARD_INPUT_DEV_H_

#define BOARD_BUTTON_RECV_BUF_SIZE 256
#define BOARD_BUTTON_SEND_BUF_SIZE 256

#define WIFI_ONOFF_KEY "wifi key"
#define SHORT_RESET_KEY "short reset key"
#define LONG_RESET_KEY "long reset key"
#define LED_ONOFF_KEY "led onoff key"
#define REBOOT_RESET_KEY "reboot reset key"



typedef struct board_button_data_s
{
   unsigned char   *data;
   size_t 	len;
} board_button_data_t;



//HGUB963XX-41 dingguangyu modify for key factory  funcation 2018.6.26 begin
extern void  set_trigger_board_input_event_handler(void *data, int len);
extern int  get_trigger_board_input_event_handler(void *data, int len);
//HGUB963XX-41 dingguangyu modify for key factory  funcation 2018.6.26 end




#endif

