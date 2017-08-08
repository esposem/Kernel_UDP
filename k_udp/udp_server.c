#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/udp.h>
#include <asm/atomic.h>
#include <linux/time.h>
#include <net/sock.h>

#include "kernel_udp.h"

static unsigned char myip[5] = {127,0,0,4,'\0'};
static int myport = 3000;
module_param(myport, int, S_IRUGO);
MODULE_PARM_DESC(myport,"The receiving port, default 3000");

static udp_service * udp_server;
static struct socket * udps_socket;

int connection_handler(void *data)
{
  struct sockaddr_in address;
  struct socket *learner_socket = udps_socket;
  int ret;
  unsigned char * in_buf = kmalloc(MAX_UDP_SIZE, GFP_KERNEL);
  unsigned char * out_buf= kmalloc(strlen(OK) +1, GFP_KERNEL);

  while (1){

    if(kthread_should_stop() || signal_pending(current)){
      check_sock_allocation(udp_server, learner_socket);
      kfree(in_buf);
      kfree(out_buf);
      return 0;
    }

    ret = udp_server_receive(learner_socket, &address, in_buf, MSG_WAITALL,udp_server);

    if(ret > 0){
      if(memcmp(in_buf, HELLO, strlen(HELLO)) == 0){
        printk(KERN_INFO "%s Got HELLO", udp_server->name);
        memset(out_buf, '\0', strlen(OK)+1);
        memcpy(out_buf, OK, strlen(OK)+1);
        ret = udp_server_send(learner_socket, &address,out_buf, strlen(OK)+1, MSG_WAITALL, udp_server->name);
        // return 0; // stop from looping
      }
    }
  }
  return 0;
}

int udp_server_listen(void)
{
  udp_server_init(udp_server, &udps_socket, myip, &myport);
  if(atomic_read(&udp_server->thread_running) == 1){
    connection_handler(NULL);
    atomic_set(&udp_server->thread_running, 0);
  }
  return 0;
}

void udp_server_start(void){
  udp_server->u_thread = kthread_run((void *)udp_server_listen, NULL, udp_server->name);
  if(udp_server->u_thread >= 0){
    atomic_set(&udp_server->thread_running,1);
    printk(KERN_INFO "%s Thread running [udp_server_start]", udp_server->name);
  }else{
    printk(KERN_INFO "%s Error in starting thread. Terminated [udp_server_start]", udp_server->name);
  }
}

static int __init network_server_init(void)
{
  udp_server = kmalloc(sizeof(udp_service), GFP_KERNEL);
  if(!udp_server){
    printk(KERN_INFO "Failed to initialize server [network_server_init]");
  }else{
    init_service(udp_server, "Server:");
    udp_server_start();
  }
  return 0;
}

static void __exit network_server_exit(void)
{
  udp_server_quit(udp_server, udps_socket);
}

module_init(network_server_init)
module_exit(network_server_exit)
MODULE_LICENSE("MIT");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
