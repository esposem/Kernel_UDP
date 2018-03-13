#ifndef U_CL_OP
#define U_CL_OP

#include "user_message.h"
#include "user_udp.h"

extern int udps_socket;

extern void echo_server(message_data* rcv_buf, message_data* rcv_check,
                        int verbose);

#endif
