#include "user_udp.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int * f;

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

int prepare_files(enum operations op, unsigned int ntests){
  if(op != PRINT){
    f = malloc(sizeof(int)*ntests);
    int n =1;
    for (size_t i = 0; i < ntests; i++) {
      char filen[100];
      snprintf(filen, 100, "./results/user_data/results%u.txt", n);
      n*=2;
      close(open(filen, O_CREAT | O_RDWR | O_TRUNC,  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH));
      f[i] = open(filen, O_CREAT | O_RDWR | O_APPEND,  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
      if(!f){
        printf("Cannot create file\n");
        f[i] = -1;
      }
    }

  }
  return 0;
}

void close_files(unsigned int nfiles){

  for (size_t i = 0; i < nfiles; i++) {
    if(f[i] != -1)
      close(f[i]);
  }
  free(f);
}
