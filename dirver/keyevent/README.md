keyevent is a module to send/receive from userspace process with block
kernel:
	get_trigger_board_input_event_handler: to get event
	set_trigger_board_input_event_handler: to send event
userspace:
	board_event:Is a process that use poll to block when receive a event then wakup
