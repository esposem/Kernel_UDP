#include <asm/atomic.h>
#include <net/sock.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <net/inet_common.h>
#include "kernel_udp.h"

int MAX_MESS_SIZE;
message_data * request;
message_data * reply;

void check_params(unsigned char * dest, unsigned int * src, int arg){
  if(arg != 4){
    if(arg != 0)
      printk(KERN_ERR "Ip not valid, using the default one");
    return;
  }
  for (size_t i = 0; i < 4; i++) {
    dest[i] = (char) src[i];
  }
  dest[4] = '\0';
}

void check_operation(enum operations * operation, char * op){
  if(op[0] == 'l' || op[0] == 'L'){
    *operation = LATENCY;
  }else if(op[0] == 't' || op[0] == 'T'){
    *operation = TROUGHPUT;
  }
}

u32 create_address(u8 *ip)
{
  u32 addr = 0;
  int i;

  for(i=0; i<4; i++)
  {
    addr += ip[i];
    if(i==3)
    break;
    addr <<= 8;
  }
  return addr;
}

void init_messages(void){
  size_t size_req = strlen(REQUEST)+1;
  size_t size_repl = strlen(REPLY)+1;
  request = kmalloc(sizeof(message_data)+ size_req, GFP_KERNEL);
  reply = kmalloc(sizeof(message_data)+ size_repl, GFP_KERNEL);

  request->mess_len = size_req;
  reply->mess_len = size_repl;
  memcpy(reply->mess_data, REPLY, size_repl);
  memcpy(request->mess_data, REQUEST, size_req);

  MAX_MESS_SIZE = max(size_req,size_repl);
}


void division(size_t dividend, size_t divisor, char * result, size_t size_res){
  snprintf(result, size_res, "%zu.%zu", dividend/divisor, (dividend*1000/divisor)%1000);
}

void check_sock_allocation(udp_service * k, struct socket * s){
  if(atomic_read(&k->socket_allocated) == 1){
    // printk(KERN_INFO "%s Released socket",k->name);
    atomic_set(&k->socket_allocated, 0);
    sock_release(s);
  }
}

void construct_header(struct msghdr * msg, struct sockaddr_in * address){

  msg->msg_name    = address;
  msg->msg_namelen = sizeof(struct sockaddr_in);
  msg->msg_control = NULL;
  msg->msg_controllen = 0;
  msg->msg_flags = 0; // this is set after receiving a message
}

// returns how many bytes are sent
int udp_send(struct socket *sock, struct msghdr * header, void * buff, size_t size_buff) {

  struct kvec vec;
  int sent, size_pkt, totbytes = 0;
  long long buffer_size = size_buff;
  char * buf = (char *) buff;

  mm_segment_t oldmm;

  while(buffer_size > 0){
    if(buffer_size < MAX_UDP_SIZE){
      size_pkt = buffer_size;
    }else{
      size_pkt = MAX_UDP_SIZE;
    }

    vec.iov_len = size_pkt;
    vec.iov_base = buf;

    buffer_size -= size_pkt;
    buf += size_pkt;

    oldmm = get_fs(); set_fs(KERNEL_DS);
    sent = kernel_sendmsg(sock, header, &vec, 1, size_pkt);
    set_fs(oldmm);

    totbytes+=sent;
  }

  return totbytes;
}

// returns the amount of data that has received
int udp_receive(struct socket *sock, struct msghdr * header, void * buff, size_t size_buff){
  struct kvec vec;
  mm_segment_t oldmm;
  int res;

  vec.iov_len = size_buff;
  vec.iov_base = buff;


  oldmm = get_fs(); set_fs(KERNEL_DS);
  // MSG_DONTWAIT: nonblocking operation: as soon as the packet is read, the call returns
  // MSG_WAITALL: blocks until it does not receive size_buff bytes OR the SO_RCVTIMEO expires.
  res =  kernel_recvmsg(sock, header, &vec, 1, size_buff, MSG_WAITALL);
  set_fs(oldmm);

  return res;
}

void udp_init(udp_service * k, struct socket ** s, unsigned char * myip, int myport){
  int server_err;
  struct socket *conn_socket;
  struct sockaddr_in server;
  struct timeval tv;
  mm_segment_t fs;

  #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,2,0)
    server_err = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, s);
  #else
    server_err = sock_create_kern(AF_INET, SOCK_DGRAM, IPPROTO_UDP, s);
  #endif
  if(server_err < 0){
    printk(KERN_INFO "%s Error %d while creating socket ",k->name, server_err);
    atomic_set(&k->thread_running, 0);
    return;
  }else{
    atomic_set(&k->socket_allocated, 1);
    printk(KERN_INFO "%s Created socket ",k->name);
  }

  conn_socket = *s;
  server.sin_addr.s_addr = htonl(create_address(myip));
  server.sin_family = AF_INET;
  server.sin_port = htons(myport);

  server_err = conn_socket->ops->bind(conn_socket, (struct sockaddr*)&server, sizeof(server));
  if(server_err < 0) {
    printk(KERN_INFO "%s Error %d while binding socket %pI4",k->name, server_err, &server.sin_addr);
    atomic_set(&k->socket_allocated, 0);
    sock_release(conn_socket);
    atomic_set(&k->thread_running, 0);
    return;
  }else{
    // get the actual random port and update myport
    int i = (int) sizeof(struct sockaddr_in);
    inet_getname(conn_socket, (struct sockaddr *) &server, &i , 0);
    myport = ntohs(server.sin_port);
    printk(KERN_INFO "%s Socket is bind to %pI4:%d\n",k->name, &server.sin_addr, myport);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    fs = get_fs();
	  set_fs(KERNEL_DS);
    kernel_setsockopt(conn_socket, SOL_SOCKET, SO_RCVTIMEO, (char * )&tv, sizeof(tv));
    set_fs(fs);
  }
}

void init_service(udp_service * k, char * name){
  memset(k, 0, sizeof(udp_service));
  atomic_set(&k->socket_allocated, 0);
  atomic_set(&k->thread_running, 0);
  size_t stlen = strlen(name) + 1;
  k->name = kmalloc(stlen, GFP_KERNEL);
  memcpy(k->name, name, stlen);
  printk(KERN_INFO "%s Initialized", k->name);
}

void udp_server_quit(udp_service * k, struct socket * s){
  int ret;
  if(k!= NULL){
    if(atomic_read(&k->thread_running) == 1){
      atomic_set(&k->thread_running, 0);
      if((ret = kthread_stop(k->u_thread)) == 0){
        printk(KERN_INFO "%s Terminated thread", k->name);
      }else{
        printk(KERN_INFO "%s Error %d in terminating thread", k->name, ret);
      }
    }else{
      printk(KERN_INFO "%s Thread was already stopped", k->name);
    }

    if(atomic_read(&k->socket_allocated) == 1){
      atomic_set(&k->socket_allocated, 0);
      sock_release(s);
      printk(KERN_INFO "%s Released socket", k->name);
    }
    printk(KERN_INFO "%s Module unloaded", k->name);
    kfree(k->name);
    kfree(k);
  }else{
    printk(KERN_INFO "Module was NULL, terminated");
  }

}
