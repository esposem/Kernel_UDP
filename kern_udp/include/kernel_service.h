#ifndef K_SERVICE
#define K_SERVICE

#include <net/sock.h>

#define SIZE_NAME 20

typedef int (*tfunc)(void*);

typedef struct udp_service udp_service;

extern void  k_thread_stop(struct udp_service* k);
extern void  init_service(udp_service** k, char* name, unsigned char* myip,
                          int myport, tfunc f, void* data);
extern void  quit_service(udp_service* k);
extern char* get_service_name(udp_service* k);
extern struct socket* get_service_sock(udp_service* k);
extern void           check_params(unsigned char* dest, char* src);
extern void           adjust_name(char* print, char* src, int size_name);

#endif
