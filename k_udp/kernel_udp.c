#include <asm/atomic.h>
#include <net/sock.h>
// #include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <net/inet_common.h>
#include "kernel_udp.h"

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

// result must be at least 256 characters!
void division(size_t dividend, size_t divisor, char * result, size_t size_res){
  // printk(KERN_INFO "%zu / %zu", dividend, divisor);
  size_t r = (dividend*1000/divisor);
  char str[size_res];
  memset(str, '\0', size_res);
  memset(result, '\0', size_res);
  int x = snprintf(str, size_res, "%zu", r);//sprintf(str, "%d", r);
  int i = 0,additional = 1;
  for (; i < x-3; i++) {
    result[i] = str[i];
  }
  if(i == 0){
    result[i] = '0';
    additional++;
    result[i+1] = ',';
  }else{
    result[i] = ',';
  }
  for (; i < x; i++) {
    result[i+additional] = str[i];
  }
  // printk(KERN_INFO "in float %s\n", result);
}

void check_sock_allocation(udp_service * k, struct socket * s){
  if(atomic_read(&k->socket_allocated) == 1){
    // printk(KERN_INFO "%s Released socket",k->name);
    atomic_set(&k->socket_allocated, 0);
    sock_release(s);
  }
}

// returns how many bytes are sent
// need to memset buffer to \0 before
// and copy data in buffer
int udp_server_send(struct socket *sock, struct sockaddr_in *address, const char *buf, const size_t buffer_size, unsigned long flags, \
   char * module_name)
{
    struct msghdr msg;
    struct kvec vec;
    int lenn, min, npacket =0, totbytes = 0;
    int l = buffer_size;

    mm_segment_t oldmm;

    msg.msg_name    = address;
    msg.msg_namelen = sizeof(struct sockaddr_in);
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = flags;

    oldmm = get_fs(); set_fs(KERNEL_DS);

    while(l > 0){
      if(l < MAX_UDP_SIZE){
        min = l;
      }else{
        min = MAX_UDP_SIZE;
      }
      vec.iov_len = min;
      vec.iov_base = (char *)buf;
      l -= min;
      buf += min;
      npacket++;

      lenn = kernel_sendmsg(sock, &msg, &vec, sizeof(vec), min);
      if(lenn > 0){
        totbytes+=lenn;
        #if TEST == 0
          printk(KERN_INFO "%s Sent message to %pI4 : %hu, size %d",module_name, &address->sin_addr, ntohs(address->sin_port), lenn);
        #endif
      }
      #if TEST == 0
        else{
          printk(KERN_INFO "ERROR IN SENDMSG");
        }
      #endif
    }

    set_fs(oldmm);

    return totbytes;
}

// buff MUST be MAX_UDP_SIZE big, so it can intercept any sized packet
// returns the amount of data that has received
// no need to memset buffer to \0 before
int udp_server_receive(struct socket *sock, struct sockaddr_in *address, unsigned char *buf, unsigned long flags,\
    udp_service * k)
{
  struct msghdr msg;
  struct kvec vec;
  int lenm;

  memset(address, 0, sizeof(struct sockaddr_in));
  memset(buf, '\0', MAX_UDP_SIZE);

  msg.msg_name = address;
  msg.msg_namelen = sizeof(struct sockaddr_in);
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = flags;

  vec.iov_len = MAX_UDP_SIZE;
  vec.iov_base = buf;

  lenm = kernel_recvmsg(sock, &msg, &vec, sizeof(vec), MAX_UDP_SIZE, flags);
  #if TEST == 0
    if(lenm > 0){
      address = (struct sockaddr_in *) msg.msg_name;
      printk(KERN_INFO "%s Received message from %pI4 : %hu , size %d ",k->name,&address->sin_addr, ntohs(address->sin_port), lenm);
    }
  #endif

  return lenm;
}

void udp_server_init(udp_service * k, struct socket ** s, unsigned char * myip, int myport){
  int server_err;
  struct socket *conn_socket;
  struct sockaddr_in server;
  struct timeval tv;
  mm_segment_t fs;
  #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,2,0)
    server_err = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, s);
  #else
  server_err = sock_create_kern(AF_INET, SOCK_DGRAM, IPPROTO_UDP, s);
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
    // printk(KERN_INFO "socket port: %pI4, %hu", &address.sin_addr, ntohs(address.sin_port) );
    printk(KERN_INFO "%s Socket is bind to %pI4 %d",k->name, &server.sin_addr, myport);
    tv.tv_sec = 0;
    tv.tv_usec = MAX_RCV_WAIT;
    fs = get_fs();
	  set_fs(KERNEL_DS);
    kernel_setsockopt(conn_socket, SOL_SOCKET, SO_RCVTIMEO, (char * )&tv, sizeof(tv));
    set_fs(fs);
    // seems not to change anything
    // int k = INT_MAX;
    // kernel_setsockopt(conn_socket, SOL_SOCKET, SO_RCVBUF, (char * )&k, sizeof(int));
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
      printk(KERN_INFO "%s Thread was not running", k->name);
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
