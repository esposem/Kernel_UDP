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
  LATENCY,
  SIMULATION
};

typedef struct message_data message_data;

extern int f;

extern int MAX_MESS_SIZE;
extern int stop;
extern message_data * request;
extern message_data *  reply;

extern void fill_sockaddr_in(struct sockaddr_in * addr, char *  ip, int flag, int port);
extern message_data * create_rcv_message(void);
extern message_data * create_message(char * data, size_t size_data, int id);
extern int get_message_id(message_data * mess);
extern void set_message_id(message_data * mess, int id);
extern char * get_message_data(message_data * mess);
extern size_t get_message_size(message_data * mess);
extern size_t get_total_mess_size(message_data * mess);
extern void delete_message(message_data * mess);
extern void del_default_messages(void);
extern void init_default_messages();
extern void construct_header(struct msghdr * msg, struct sockaddr_in * address);
extern void fill_hdr(struct msghdr * hdr,  struct iovec * iov, void * data, size_t len);
extern int prepare_file(enum operations op, unsigned int nclients);


#endif
