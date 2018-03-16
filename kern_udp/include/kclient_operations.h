#ifndef K_CL_OP
#define K_CL_OP

#include "kernel_message.h"
#include "kernel_service.h"
#include <net/sock.h>

struct common_data
{
  char            average[256];
  size_t          size_avg;
  struct timespec old_time;
  struct timespec current_time;
};

extern udp_service*       cl_thread_1;
extern unsigned long long sent;

extern void run_outstanding_clients(message_data*       rcv_buf,
                                    message_data*       send_buf,
                                    struct sockaddr_in* dest_addr,
                                    unsigned int        nclients,
                                    unsigned int duration, struct file* f);

#endif
