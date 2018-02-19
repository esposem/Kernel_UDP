#ifndef KERN_UDP
#define KERN_UDP

#include <net/sock.h>


#define MAX_UDP_SIZE 65507

extern void construct_header(struct msghdr * msg, struct sockaddr_in * address);
extern u32 create_address(u8 *ip);

extern int udp_send(struct socket *sock, struct msghdr * header, void * buff, size_t size_buff);
extern int udp_receive(struct socket *sock, struct msghdr * header, void * buff, size_t size_buff);

extern int udp_init(struct socket ** s, unsigned char * myip, int myport);
extern int sock_allocated(struct socket * s);
extern void release_socket(struct socket * s);

#endif
