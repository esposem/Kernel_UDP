#ifndef K_CL_OP
#define K_CL_OP

#include "kernel_udp.h"

struct common_data{
  char average[256];
  size_t size_avg;
  struct timespec old_time;
  struct timespec current_time;
};

extern udp_service * udp_client;
extern struct socket * udpc_socket;
extern unsigned long long sent;

extern void troughput(message_data * send_buf, struct sockaddr_in * dest_addr, unsigned long frac_sec, long tsec);
extern void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr);
extern void print(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr);

#endif
