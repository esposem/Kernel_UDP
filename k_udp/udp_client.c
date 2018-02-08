#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
// #include <linux/udp.h>
// #include <asm/atomic.h>
// #include <linux/time.h>
// #include <net/sock.h>
#include "kernel_udp.h"
#include "kclient_operations.h"

//############## CLIENT IP AND PORT (this module) #######
static unsigned char ipmy[5] = {127,0,0,1,'\0'};
static unsigned int myip[5];
static int margs;
module_param_array(myip, int, &margs, S_IRUGO);
MODULE_PARM_DESC(myip,"The client ip, default 127.0.0.1");

static int myport = 0; //random port
module_param(myport, int, S_IRUGO);
MODULE_PARM_DESC(myport,"The receiving port, default is a free random one chosen by system");
//######################################################

//############## SERVER IP AND PORT (udp_server) #######
static unsigned char serverip[5] = {127,0,0,1,'\0'};
static unsigned int destip[5];
static int sargs;
module_param_array(destip, int, &sargs, S_IRUGO);
MODULE_PARM_DESC(destip,"The server ip, default 127.0.0.1");

static int destport = 3000;
module_param(destport, int, S_IRUGO);
MODULE_PARM_DESC(destport,"The server port, default 3000");
//######################################################

//############## Types of operation #######
static char * opt = "p";
static enum operations operation = PRINT;
module_param(opt, charp, S_IRUGO);
MODULE_PARM_DESC(opt,"P or p for HELLO-OK, T or t for Troughput, L or l for Latency");

static unsigned long us = 1;
module_param(us,long, S_IRUGO);
MODULE_PARM_DESC(us,"Fraction of second between a send and the next (Throughput mode only)");
//######################################################

udp_service * udp_client;
struct socket * udpc_socket;

int connection_handler(void) {

  struct sockaddr_in dest_addr;
  struct msghdr hdr;

  init_messages();
  message_data * rcv_buff = kmalloc(sizeof(message_data) + MAX_UDP_SIZE, GFP_KERNEL);
  message_data * send_buff = kmalloc(sizeof(message_data) + MAX_MESS_SIZE, GFP_KERNEL);

  rcv_buff->mess_len = MAX_UDP_SIZE;
  send_buff->mess_len = MAX_MESS_SIZE;
  memset(send_buff->mess_data, '\0', send_buff->mess_len);
  memcpy(send_buff->mess_data, request->mess_data, request->mess_len);

  dest_addr.sin_addr.s_addr = htonl(create_address(serverip));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(destport);

  construct_header(&hdr, &dest_addr);

  #if 0
  int resc =  kernel_connect(client_socket, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr),0);
  if(resc < 0){
    printk(KERN_INFO "Error %d", -resc);
    check_sock_allocation(udp_client, client_socket);
    atomic_set(&udp_client->thread_running, 0);
    kfree(rcv_buff);
    kfree(send_buff);
    return 0;
  }
  #endif

  switch(operation){
    case LATENCY:
      latency(rcv_buff, send_buff, reply, &hdr);
      break;
    case TROUGHPUT:
      troughput(send_buff, &hdr, us);
      break;
    default:
      print(rcv_buff, send_buff, reply, &hdr);
      break;
  }

  check_sock_allocation(udp_client, udpc_socket);
  kfree(rcv_buff);
  kfree(send_buff);

  return 0;
}

int client_listen(void) {
  udp_server_init(udp_client, &udpc_socket, ipmy, myport);
  if(atomic_read(&udp_client->thread_running) == 1){
    connection_handler();
    atomic_set(&udp_client->thread_running, 0);
  }
  return 0;
}

void client_start(void){
  udp_client->u_thread = kthread_run((void *)client_listen, NULL, udp_client->name);
  if(udp_client->u_thread >= 0){
    atomic_set(&udp_client->thread_running,1);
    printk(KERN_INFO "%s Thread running", udp_client->name);
  }else{
    printk(KERN_INFO "%s Error in starting thread. Terminated", udp_client->name);
  }
}

static int __init client_init(void) {
  udp_client = kmalloc(sizeof(udp_service), GFP_KERNEL);
  if(!udp_client){
    printk(KERN_INFO "Failed to initialize CLIENT");
  }else{
    check_params(ipmy, myip, margs);
    check_params(serverip, destip, sargs);
    check_operation(&operation, opt);
    init_service(udp_client, "Client:");
    printk(KERN_INFO "%s opt: %c, us: %lu\n",udp_client->name, opt[0], us);
    client_start();
  }
  return 0;
}

static void __exit client_exit(void) {
  size_t len = strlen(udp_client->name)+1;
  char prints[len];
  memcpy(prints,udp_client->name,len);
  udp_server_quit(udp_client, udpc_socket);
  if(operation == TROUGHPUT){
    printk(KERN_INFO "%s Sent total of %llu packets",prints, sent);
  }
}

module_init(client_init)
module_exit(client_exit)
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
