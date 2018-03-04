#include "user_udp.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int f = -1;

void fill_sockaddr_in(struct sockaddr_in * addr, char *  ip, int flag, int port){
  addr->sin_addr.s_addr = inet_addr(ip);
  addr->sin_family = flag;
  addr->sin_port = htons(port);
}

void construct_header(struct msghdr * msg, struct sockaddr_in * address){
  msg->msg_name = address;
  msg->msg_namelen = sizeof(struct sockaddr_in);
  msg->msg_control = NULL;
  msg->msg_controllen = 0;
  msg->msg_flags = 0;
}

void fill_hdr(struct msghdr * hdr,  struct iovec * iov, void * data, size_t len){
  hdr->msg_iovlen = 1;
  iov->iov_base = data;
  iov->iov_len = len;
  hdr->msg_iov = iov;
}

int prepare_file(enum operations op, unsigned int nclients){
  if(op != PRINT){
    char filen[100];
    snprintf(filen, 100, "./results/user_data/results%u.txt", nclients);
    close(open(filen, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU));
    f = open(filen, O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
    if(!f){
      printf("Cannot create file\n");
      f = -1;
      return -1;
    }
  }
  return 0;
}
