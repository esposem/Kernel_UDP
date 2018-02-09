#ifndef K_CL_OP
#define K_CL_OP

#include "kernel_udp.h"

extern udp_service * udp_client;
extern struct socket * udpc_socket;
extern unsigned long long sent;

extern void troughput(message_data * send_buf, struct sockaddr_in * dest_addr, unsigned long frac_sec);
extern void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr);
extern void print(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr);

#endif
