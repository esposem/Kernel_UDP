#ifndef U_CL_OP
#define U_CL_OP

#include "user_udp.h"
#include "user_message.h"

extern int udps_socket;
extern unsigned long long received;

extern void troughput(message_data * rcv_buf, message_data * rcv_check);
extern void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check);
extern void print(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check);
extern void server_simulation(message_data * rcv_buf, message_data * rcv_check);

#endif
