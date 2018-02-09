#include "user_udp.h"

int MAX_MESS_SIZE;
message_data * request;
message_data * reply;

void init_messages(){
  size_t size_req = strlen(REQUEST)+1;
  size_t size_repl = strlen(REPLY)+1;
  request = malloc(sizeof(message_data)+ size_req);
  reply = malloc(sizeof(message_data)+ size_repl);

  request->mess_len = size_req;
  reply->mess_len = size_repl;
  memcpy(reply->mess_data, REPLY, size_repl);
  memcpy(request->mess_data, REQUEST, size_req);

  MAX_MESS_SIZE = size_req > size_repl ? size_req : size_repl;
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
