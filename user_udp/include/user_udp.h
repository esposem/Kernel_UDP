#ifndef USER_UDP
#define USER_UDP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>

#define MAX_UDP_SIZE 65507
#define _1_SEC_TO_NS 1000000000
// sometimes the rcv blocks for less than 1 sec, so allow this error
#define ABS_ERROR 2000000

enum operations {
  PRINT,
  TROUGHPUT,
  LATENCY,
  SIMULATION
};

extern int * f;
extern int stop;

extern void fill_sockaddr_in(struct sockaddr_in * addr, char *  ip, int flag, int port);
extern void construct_header(struct msghdr * msg, struct sockaddr_in * address);
extern void fill_hdr(struct msghdr * hdr,  struct iovec * iov, void * data, size_t len);
extern int prepare_files(enum operations op, unsigned int nclients);
extern void close_files(unsigned int nfiles);


#endif
