#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <net/udp.h>
#include <asm/atomic.h>
#include <linux/time.h>
#include <net/sock.h>

#include "kernel_udp.h"

static unsigned char acceptorip[5] = {127,0,0,3,'\0'};
static int acceptorport = 3003;
static unsigned char myip[5] = {127,0,0,2,'\0'};
static int myport = 3002;
static struct in_addr clientaddr;
static unsigned short clientport;

module_param(myport, int, S_IRUGO);
MODULE_PARM_DESC(myport,"The receiving port, default 3002");

static int len = 50;
module_param(len, int, S_IRUGO);
MODULE_PARM_DESC(len,"Data packet length, default 50, max 65507 (automatically added space for terminating 0)");

static udp_service * kproposer;
static struct socket * kpsocket;

int connection_handler(void *data)
{
  struct sockaddr_in address;
  struct socket *proposer_socket = kpsocket;

  int ret;
  unsigned char * in_buf = kmalloc(len, GFP_KERNEL);
  unsigned char * out_buf = kmalloc(len, GFP_KERNEL);
  size_t size_buf, size_msg1, size_msg2, size_msg3;


  size_msg1 = strlen(VFC);
  size_msg2 = strlen(P1B);
  size_msg3 = strlen(A2B);

  while (1){

    if(kthread_should_stop() || signal_pending(current)){
      check_sock_allocation(kproposer, proposer_socket);
      kfree(in_buf);
      kfree(out_buf);
      return 0;
    }

    memset(in_buf, '\0', len);
    memset(&address, 0, sizeof(struct sockaddr_in));
    ret = udp_server_receive(proposer_socket, &address, in_buf, len, MSG_WAITALL, kproposer);
    if(ret > 0){
      size_buf = strlen(in_buf);
      if(memcmp(in_buf, VFC, size_buf > size_msg1 ? size_msg1 : size_buf) == 0){
        memcpy(&clientaddr, &address.sin_addr, sizeof(struct in_addr));
        unsigned short i = ntohs(address.sin_port);
        memcpy(&clientport, &i, sizeof(unsigned short));
        address.sin_addr.s_addr = htonl(create_address(acceptorip));
        _send_message(proposer_socket, &address, out_buf, acceptorport, P1A, len, kproposer->name);

      }else if (memcmp(in_buf, P1B, size_buf > size_msg2 ? size_msg2 : size_buf) == 0){
        // address.sin_addr.s_addr = htonl(create_address(acceptorip));
        _send_message(proposer_socket, &address, out_buf, acceptorport, A2A, len, kproposer->name);

      } else if (memcmp(in_buf, A2B, size_buf > size_msg3 ? size_msg3 : size_buf) == 0){
        address.sin_addr = clientaddr;
        _send_message(proposer_socket, &address, out_buf, clientport, "ALL DONE", len, kproposer->name);

      }

    }
  }

  return 0;
}

int udp_server_listen(void)
{
  udp_server_init(kproposer, &kpsocket, myip, &myport);
  connection_handler(NULL);
  atomic_set(&kproposer->thread_running, 0);
  return 0;
}

void udp_server_start(void){
  kproposer->u_thread = kthread_run((void *)udp_server_listen, NULL, kproposer->name);
  if(kproposer->u_thread >= 0){
    atomic_set(&kproposer->thread_running,1);
    printk(KERN_INFO "%s Thread running [udp_server_start]", kproposer->name);
  }else{
    printk(KERN_INFO "%s Error in starting thread. Terminated [udp_server_start]", kproposer->name);
  }
}

static int __init network_server_init(void)
{
  kproposer = kmalloc(sizeof(udp_service), GFP_KERNEL);
  if(!kproposer){
    printk(KERN_INFO "Failed to initialize server [network_server_init]");
  }else{
    init_service(kproposer, "Proposer:", &len);
    udp_server_start();
  }
  return 0;
}

static void __exit network_server_exit(void)
{
  udp_server_quit(kproposer, kpsocket);
}

module_init(network_server_init)
module_exit(network_server_exit)
MODULE_LICENSE("MIT");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
