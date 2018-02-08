#ifndef K_SER_OP
#define K_SER_OP

#include "kernel_udp.h"

extern udp_service * udp_server;
extern struct socket * udps_socket;
extern unsigned long long received;

extern void troughput(message_data * rcv_buf, message_data * rcv_check, struct msghdr * hdr);
extern void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct msghdr * hdr);
extern void print(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct msghdr * hdr);

#endif
