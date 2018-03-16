#include "kernel_udp.h"
#include <linux/slab.h>
#include <linux/version.h>
#include <net/inet_common.h>

// Handle UDP connection and socket status.

void
construct_header(struct msghdr* msg, struct sockaddr_in* address)
{
  msg->msg_name = address;
  msg->msg_namelen = sizeof(struct sockaddr_in);
  msg->msg_control = NULL;
  msg->msg_controllen = 0;
  msg->msg_flags = 0; // this is set after receiving a message
}

u32
create_address(u8* ip)
{
  u32 addr = 0;
  int i;

  for (i = 0; i < 4; i++) {
    addr += ip[i];
    if (i == 3)
      break;
    addr <<= 8;
  }
  return addr;
}

// returns how many bytes are sent
int
udp_send(struct socket* sock, struct msghdr* header, void* buff,
         size_t size_buff)
{

  struct kvec vec;
  int         sent, size_pkt, totbytes = 0;
  long long   buffer_size = size_buff;
  char*       buf = (char*)buff;

  mm_segment_t oldmm;

  while (buffer_size > 0) {
    if (buffer_size < MAX_UDP_SIZE) {
      size_pkt = buffer_size;
    } else {
      size_pkt = MAX_UDP_SIZE;
    }

    vec.iov_len = size_pkt;
    vec.iov_base = buf;

    buffer_size -= size_pkt;
    buf += size_pkt;

    oldmm = get_fs();
    set_fs(KERNEL_DS);
    sent = kernel_sendmsg(sock, header, &vec, 1, size_pkt);
    set_fs(oldmm);

    totbytes += sent;
  }

  return totbytes;
}

// returns the amount of data that has received
int
udp_receive(struct socket* sock, struct msghdr* header, void* buff,
            size_t size_buff)
{
  struct kvec  vec;
  mm_segment_t oldmm;
  int          res;

  vec.iov_len = size_buff;
  vec.iov_base = buff;

  oldmm = get_fs();
  set_fs(KERNEL_DS);
  // MSG_DONTWAIT: nonblocking operation: as soon as the packet is read, the
  // call returns MSG_WAITALL: blocks until it does not receive size_buff bytes
  // OR the SO_RCVTIMEO expires.
  res = kernel_recvmsg(sock, header, &vec, 1, size_buff, MSG_WAITALL);
  set_fs(oldmm);

  return res;
}

int
udp_init(struct socket** s, unsigned char* myip, int myport)
{
  int                err;
  struct socket*     conn_sock;
  struct sockaddr_in address;
  mm_segment_t       fs;
  struct timeval     tv = { 0, 100000 };
  int                flag = 1;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
  err = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, s);
#else
  err = sock_create_kern(AF_INET, SOCK_DGRAM, IPPROTO_UDP, s);
#endif

  if (err < 0) {
    printk(KERN_ERR "Error %d while creating socket\n", err);
    *s = NULL;
    return err;
  }
  conn_sock = *s;

  printk(KERN_INFO "Socket created\n");

  fs = get_fs();
  set_fs(KERNEL_DS);
  kernel_setsockopt(conn_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
  kernel_setsockopt(conn_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&flag,
                    sizeof(int));
  kernel_setsockopt(conn_sock, SOL_SOCKET, SO_REUSEPORT, (char*)&flag,
                    sizeof(int));
  set_fs(fs);

  address.sin_addr.s_addr = htonl(create_address(myip));
  address.sin_family = AF_INET;
  address.sin_port = htons(myport);

  err = conn_sock->ops->bind(conn_sock, (struct sockaddr*)&address,
                             sizeof(address));
  if (err < 0) {
    printk(KERN_ERR "Error %d while binding socket to %pI4\n", err,
           &address.sin_addr);
    sock_release(conn_sock);
    *s = NULL;
    return err;
  } else {
    // get the actual random port and update myport
    int i = (int)sizeof(struct sockaddr_in);
    inet_getname(conn_sock, (struct sockaddr*)&address, &i, 0);
    printk(KERN_INFO "Socket is bind to %pI4:%d\n", &address.sin_addr,
           ntohs(address.sin_port));
  }
  return 0;
}

int
sock_allocated(struct socket* s)
{
  return s != NULL;
}

void
release_socket(struct socket* s)
{
  if (s)
    sock_release(s);
}
