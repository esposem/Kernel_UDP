#include "user_udp.h"

int MAX_MESS_SIZE;
message_data * request;
message_data * reply;

struct message_data{
  int id;
  size_t mess_len;
  unsigned char mess_data[0];
};

void fill_sockaddr_in(struct sockaddr_in * addr, unsigned char *  ip, int flag, int port){
  addr->sin_addr.s_addr = inet_addr(ip);
  addr->sin_family = flag;
  addr->sin_port = htons(port);
}


message_data * create_rcv_message(void){
  return create_message(NULL, MAX_MESS_SIZE, -1);
}

message_data * create_message(char * data, size_t size_data, int id){
  message_data * res = malloc(sizeof(struct message_data) + size_data);
  memset(res,'\0',sizeof(struct message_data) + size_data);
  res->mess_len = size_data;
  res->id = id;
  if(data)
    memcpy(res->mess_data, data, size_data);
  return res;
}

int get_message_id(message_data * mess){
  return mess->id;
}

void set_message_id(message_data * mess, int id){
  mess->id = id;
}

char * get_message_data(message_data * mess){
  return mess->mess_data;
}


size_t get_message_size(message_data * mess){
  return mess->mess_len;
}

size_t get_total_mess_size(message_data * mess){
  return mess->mess_len + sizeof(struct message_data);
}


void delete_message(message_data * mess){
  if(mess)
    free(mess);
}

void del_default_messages(void){
  free(request);
  free(reply);
}

void init_default_messages(){
  size_t size_req = strlen(REQUEST)+1;
  size_t size_repl = strlen(REPLY)+1;

  MAX_MESS_SIZE = size_req > size_repl ? size_req : size_repl;
  request = create_message(NULL, MAX_MESS_SIZE, -1);
  reply = create_message(NULL, MAX_MESS_SIZE, -1);

  memset(reply->mess_data, '\0', MAX_MESS_SIZE);
  memset(request->mess_data, '\0', MAX_MESS_SIZE);

  memcpy(reply->mess_data, REPLY, size_repl);
  memcpy(request->mess_data, REQUEST, size_req);
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
