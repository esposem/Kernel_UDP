#ifndef K_CL_OP
#define K_CL_OP

#include <net/sock.h>
#include "kernel_message.h"
#include "kernel_service.h"

struct common_data{
  char average[256];
  size_t size_avg;
  struct timespec old_time;
  struct timespec current_time;
};

extern udp_service * cl_thread_1;
extern udp_service * cl_thread_2;
extern unsigned long long sent;

extern void troughput(message_data * send_buf, struct sockaddr_in * dest_addr, unsigned long frac_sec, long tsec);
extern void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr);
extern void print(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr);
extern void client_simulation(message_data * rcv_buf, message_data * send_buf, struct sockaddr_in * dest_addr, unsigned int nclients, unsigned int ntests);

#endif
