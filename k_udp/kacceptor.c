#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <net/udp.h>
#include <asm/atomic.h>
#include <linux/time.h>
#include <net/sock.h>

#include "kernel_udp.h"

static unsigned char learnerip[5] = {127,0,0,4,'\0'};
static int learnerport = 3000;
static unsigned char myip[5] = {127,0,0,3,'\0'};
static int myport = 3003;
static struct in_addr proposeraddr;
static unsigned short proposerport;

module_param(myport, int, S_IRUGO);
MODULE_PARM_DESC(myport,"The receiving port, default 3003");

static int len = 50;
module_param(len, int, S_IRUGO);
MODULE_PARM_DESC(len,"Data packet length, default 50, max 65507 (automatically added space for terminating 0)");

static udp_service * kacceptor;
static struct socket * kasocket = NULL;

int connection_handler(void *data)
{
  struct sockaddr_in address;
  struct socket *accept_socket = kasocket;
  int ret;

  unsigned char * in_buf = kmalloc(len, GFP_KERNEL);
  unsigned char * out_buf = kmalloc(len, GFP_KERNEL);
  size_t size_msg, size_msg1, size_buf;


  size_msg = strlen(P1A);
  size_msg1 = strlen(A2A);

  while (1){

    if(kthread_should_stop() || signal_pending(current)){
      check_sock_allocation(kacceptor, accept_socket);
      kfree(in_buf);
      kfree(out_buf);
      return 0;
    }

    memset(in_buf, '\0', len);
    memset(&address, 0, sizeof(struct sockaddr_in));
    ret = udp_server_receive(accept_socket, &address, in_buf, len, MSG_WAITALL, kacceptor);
    if(ret > 0){
      size_buf = strlen(in_buf);
      if(memcmp(in_buf, P1A, size_buf > size_msg ? size_msg : size_buf) == 0){
        memcpy(&proposeraddr, &address.sin_addr, sizeof(struct in_addr));
        unsigned short i = ntohs(address.sin_port);
        memcpy(&proposerport, &i, sizeof(unsigned short));
        // address is same as receiver
        _send_message(accept_socket, &address, out_buf, proposerport, P1B, len, kacceptor->name);
      }else if (memcmp(in_buf, A2A, size_buf > size_msg1 ? size_msg1 : size_buf) == 0){
        // address is same as receiver
        _send_message(accept_socket, &address, out_buf, proposerport, A2B, len, kacceptor->name);
        address.sin_addr.s_addr = htonl(create_address(learnerip));
        _send_message(accept_socket, &address, out_buf, learnerport, A2B, len, kacceptor->name);
      }else{
        printk(KERN_INFO "%s Received %s?", kacceptor->name, in_buf);
      }
    }
  }

  return 0;
}

int udp_server_listen(void)
{
  udp_server_init(kacceptor, &kasocket, myip, &myport);
  connection_handler(NULL);
  atomic_set(&kacceptor->thread_running, 0);
  return 0;
}

void udp_server_start(void){
  kacceptor->u_thread = kthread_run((void *)udp_server_listen, NULL, kacceptor->name);
  if(kacceptor->u_thread >= 0){
    atomic_set(&kacceptor->thread_running,1);
    printk(KERN_INFO "%s Thread running [udp_server_start]", kacceptor->name);
  }else{
    printk(KERN_INFO "%s Error in starting thread. Terminated [udp_server_start]", kacceptor->name);
  }
}

static int __init network_server_init(void)
{
  kacceptor = kmalloc(sizeof(udp_service), GFP_KERNEL);
  if(!kacceptor){
    printk(KERN_INFO "Failed to initialize ACCEPTOR [network_server_init]");
  }else{
    init_service(kacceptor, "Acceptor:", &len);
    udp_server_start();
  }
  return 0;
}

static void __exit network_server_exit(void)
{
  udp_server_quit(kacceptor, kasocket);
}


module_init(network_server_init)
module_exit(network_server_exit)
MODULE_LICENSE("MIT");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
