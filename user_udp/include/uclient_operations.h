#ifndef U_CL_OP
#define U_CL_OP

#include "user_message.h"
#include "user_udp.h"

extern int udpc_socket;

extern void run_outstanding_clients(message_data*       rcv_buf,
                                    message_data*       send_buf,
                                    struct sockaddr_in* dest_addr,
                                    unsigned int nclients, long duration,
                                    int verbose);

#endif
