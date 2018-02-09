#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/udp.h>
#include <asm/atomic.h>
#include <linux/time.h>
#include <net/sock.h>
#include "kernel_udp.h"
#include "kserver_operations.h"

//############## SERVER IP AND PORT (this module) #######
static unsigned char ipmy[5] = {127,0,0,1,'\0'};
static unsigned int myip[5];
static int margs;
module_param_array(myip, int, &margs, S_IRUGO);
MODULE_PARM_DESC(myip,"The server ip, default 127.0.0.4");

static int myport = 3000;
module_param(myport, int, S_IRUGO);
MODULE_PARM_DESC(myport,"The receiving port, default 3000");
//######################################################

//############## Types of operation #######
static char * opt = "p";
static enum operations operation = PRINT;
module_param(opt, charp, S_IRUGO);
MODULE_PARM_DESC(opt,"P or p for HELLO-OK, T or t for Troughput, L or l for Latency");
//######################################################

udp_service * udp_server;
struct socket * udps_socket;

static void connection_handler(void){
  
  init_messages();
  message_data * rcv_buff = kmalloc(sizeof(message_data) + MAX_UDP_SIZE, GFP_KERNEL);
  message_data * send_buff = kmalloc(sizeof(message_data) + MAX_MESS_SIZE, GFP_KERNEL);

  rcv_buff->mess_len = MAX_UDP_SIZE;
  send_buff->mess_len = MAX_MESS_SIZE;
  memcpy(send_buff->mess_data, reply->mess_data, reply->mess_len);

  switch(operation){
    case LATENCY:
      latency(rcv_buff, send_buff, request);
      break;
    case TROUGHPUT:
      troughput(rcv_buff, request);
      break;
    default:
      print(rcv_buff, send_buff, request);
      break;
  }

}

static int server_listen(void){
  udp_init(udp_server, &udps_socket, ipmy, myport);
  if(atomic_read(&udp_server->thread_running) == 1){
    connection_handler();
    atomic_set(&udp_server->thread_running, 0);
  }
  return 0;
}

static void server_start(void){
  udp_server->u_thread = kthread_run((void *)server_listen, NULL, udp_server->name);
  if(udp_server->u_thread >= 0){
    atomic_set(&udp_server->thread_running,1);
    printk(KERN_INFO "%s Thread running", udp_server->name);
  }else{
    printk(KERN_INFO "%s Error in starting thread. Terminated", udp_server->name);
  }
}

static int __init server_init(void){
  udp_server = kmalloc(sizeof(udp_service), GFP_KERNEL);
  if(!udp_server){
    printk(KERN_INFO "Failed to initialize server");
  }else{
    check_params(ipmy, myip, margs);
    check_operation(&operation, opt);
    init_service(udp_server, "Server:");
    printk(KERN_INFO "%s opt: %c\n",udp_server->name, opt[0]);
    server_start();
  }
  return 0;

}

static void __exit server_exit(void){
  size_t len = strlen(udp_server->name)+1;
  char prints[len];
  memcpy(prints,udp_server->name,len);
  udp_server_quit(udp_server, udps_socket);
  if(operation == TROUGHPUT){
    printk(KERN_INFO "%s Received total of %llu packets", prints, received);
  }
}

module_init(server_init)
module_exit(server_exit)
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
