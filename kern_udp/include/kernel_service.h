#ifndef K_SERVICE
#define K_SERVICE

#include <net/sock.h>
#include <linux/kthread.h>

#define SIZE_NAME 20

enum operations {
  PRINT,
  TROUGHPUT,
  LATENCY,
  SIMULATION
};

typedef struct udp_service udp_service;

extern struct file ** f;
extern void k_thread_stop(struct udp_service * k);

extern void init_service(udp_service ** k, char * name, unsigned char * myip, int myport, int(*funct)(void), void * data);
extern void quit_service(udp_service * k);
extern char * get_service_name(udp_service * k);
extern struct socket * get_service_sock(udp_service * k);

extern void check_operation(enum operations * operation, char * op);
extern void check_params(unsigned char * dest, char * src);
extern void adjust_name(char * print, char * src, int size_name);

extern int prepare_files(enum operations op, unsigned int ntests);
extern void close_files(unsigned int nfiles);

#endif
