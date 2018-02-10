#ifndef KERN_UDP
#define KERN_UDP

#include <linux/udp.h>
#include <linux/version.h>

struct udp_service
{
  // struct socket * u_socket;
  struct task_struct * u_thread;
  char * name;
  atomic_t thread_running;  //1 yes 0 no
  atomic_t socket_allocated;//1 yes 0 no
};

struct message_data{
  size_t mess_len;
  unsigned char mess_data[0];
};

enum operations {
  PRINT,
  TROUGHPUT,
  LATENCY
};

typedef struct udp_service udp_service;
typedef struct message_data message_data;

#define MAX_UDP_SIZE 65507
#define _1_SEC_TO_NS 1000000000

#define REQUEST "HELLO"
#define REPLY "OK"
// sometimes the rcv blocks for less than 1 sec, so allow this error
#define ABS_ERROR 2000000

extern int MAX_MESS_SIZE;
extern message_data * request;
extern message_data *  reply;

extern void init_messages(void);
extern void construct_header(struct msghdr * msg, struct sockaddr_in * address);
extern u32 create_address(u8 *ip);

extern int udp_send(struct socket *sock, struct msghdr * header, void * buff, size_t size_buff);
extern int udp_receive(struct socket *sock, struct msghdr * header, void * buff, size_t size_buff);

extern void udp_init(udp_service * k, struct socket ** s, unsigned char * myip, int myport);
extern void init_service(udp_service * k, char * name);
extern void udp_server_quit(udp_service * k, struct socket * s);

extern void check_sock_allocation(udp_service * k, struct socket * s);
extern void check_params(unsigned char * dest, unsigned int * src, int arg);
extern void check_operation(enum operations * operation, char * operations);
extern void division(size_t dividend, size_t divisor, char * result, size_t size_res);

#endif
