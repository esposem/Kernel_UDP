#ifndef K_SER_OP
#define K_SER_OP

#include <net/sock.h>
#include "kernel_message.h"
#include "kernel_service.h"

extern udp_service * udp_server;
extern unsigned long long received;

extern void troughput(message_data * rcv_buf, message_data * rcv_check);
extern void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check);
extern void print(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check);
extern void server_simulation(message_data * rcv_buf, message_data * rcv_check);

#endif
