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

#define REQUEST "HELLO"
#define REPLY "OK"
#define MAX_UDP_SIZE 65507
#define _1_SEC_TO_NS 1000000000
// sometimes the rcv blocks for less than 1 sec, so allow this error
#define ABS_ERROR 2000000

enum operations {
  PRINT,
  TROUGHPUT,
  LATENCY
};

struct message_data{
  size_t mess_len;
  unsigned char mess_data[0];
};

typedef struct message_data message_data;


extern int MAX_MESS_SIZE;
extern int stop;
extern message_data * request;
extern message_data *  reply;

extern void init_messages(void);
extern void construct_header(struct msghdr * msg, struct sockaddr_in * address);
extern void fill_hdr(struct msghdr * header,  struct iovec * iov, void * data, size_t len);


#endif
