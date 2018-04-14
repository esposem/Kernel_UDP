#include "kernel_message.h"
#include "kernel_udp.h"

// related to throughput and latency tests. Shared between server and client
// check thread ps --ppid 2 -p 2 -o uname,pid,ppid,cmd,cls

int MAX_MESS_SIZE;
message_data *request;
message_data *reply;
message_data *test;

struct message_data {
  int id;
  size_t mess_len;
  char mess_data[0];
};

message_data *create_rcv_message(void) {
  return create_message(NULL, MAX_MESS_SIZE, -1);
}

message_data *create_message(char *data, size_t size_data, int id) {
  message_data *res =
      kmalloc(sizeof(struct message_data) + size_data, GFP_KERNEL);
  memset(res, '\0', sizeof(struct message_data) + size_data);
  res->mess_len = size_data;
  res->id = id;
  if (data)
    memcpy(res->mess_data, data, size_data);
  return res;
}

int get_message_id(message_data *mess) { return mess->id; }

void set_message_id(message_data *mess, int id) { mess->id = id; }

char *get_message_data(message_data *mess) { return mess->mess_data; }

size_t get_message_size(message_data *mess) { return mess->mess_len; }

size_t get_total_mess_size(message_data *mess) {
  return mess->mess_len + sizeof(struct message_data);
}

void delete_message(message_data *mess) {
  if (mess)
    kfree(mess);
}

void init_default_messages(void) {
  size_t size_req = strlen(REQUEST) + 1;
  size_t size_repl = strlen(REPLY) + 1;

  MAX_MESS_SIZE = max(size_req, size_repl);
  request = create_message(NULL, MAX_MESS_SIZE, -1);
  reply = create_message(NULL, MAX_MESS_SIZE, -1);

  memset(reply->mess_data, '\0', MAX_MESS_SIZE);
  memset(request->mess_data, '\0', MAX_MESS_SIZE);

  memcpy(reply->mess_data, REPLY, size_repl);
  memcpy(request->mess_data, REQUEST, size_req);
}

void del_default_messages(void) {
  kfree(request);
  kfree(reply);
}

void fill_sockaddr_in(struct sockaddr_in *addr, unsigned char *ip, int flag,
                      int port) {
  addr->sin_addr.s_addr = htonl(create_address(ip));
  addr->sin_family = flag;
  addr->sin_port = htons(port);
}

void division(size_t dividend, size_t divisor, char *result, size_t size_res) {
  if (divisor > 0) {
    snprintf(result, size_res, "%zu.%zu", dividend / divisor,
             (dividend * 1000 / divisor) % 1000);
  }
}
