#ifndef USER_UDP
#define USER_UDP

#include <arpa/inet.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

extern int stop;

extern void fill_sockaddr_in(struct sockaddr_in* addr, const char* ip, int flag,
                             int port);
extern void construct_header(struct msghdr* msg, struct sockaddr_in* address);
extern void fill_hdr(struct msghdr* hdr, struct iovec* iov, void* data,
                     size_t len);

#endif
