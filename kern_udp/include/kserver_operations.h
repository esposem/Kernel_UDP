#ifndef K_SER_OP
#define K_SER_OP

#include "kernel_message.h"
#include "kernel_service.h"

extern udp_service* udp_server;

extern void echo_server(message_data* rcv_buf, message_data* rcv_check);

#endif
