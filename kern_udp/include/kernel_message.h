#ifndef K_MESS
#define K_MESS

#include <net/sock.h>

typedef struct message_data message_data;

// sometimes the rcv blocks for less than 1 sec, so allow this error
#define ABS_ERROR 2000000
#define _1_SEC_TO_NS 1000000000
#define REQUEST "HELLO"
#define REPLY "OK"
#define TEST "TESTL"

extern int MAX_MESS_SIZE;
extern message_data * request;
extern message_data *  reply;
extern message_data *  test;

extern void init_default_messages(void);
extern void del_default_messages(void);

extern void fill_sockaddr_in(struct sockaddr_in * addr, unsigned char *  ip, int flag, int port);
extern void division(size_t dividend, size_t divisor, char * result, size_t size_res);

extern message_data * create_message(size_t size_data, int recv);
extern char * get_message_data(message_data * mess);
extern size_t get_message_size(message_data * mess);
extern void delete_message(message_data * mess);

#endif
