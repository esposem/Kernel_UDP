#ifndef U_CL_OP
#define U_CL_OP

#include "user_udp.h"
#include "user_message.h"

extern int udpc_socket;
extern unsigned long long sent;

extern void troughput(message_data * send_buf, struct sockaddr_in * dest_addr, unsigned long frac_sec, long tsec);
extern void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr);
extern void print(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr);
extern void client_simulation(message_data * rcv_buf, message_data * send_buf, struct sockaddr_in * dest_addr, unsigned int nclients, unsigned int nfile);

#endif
