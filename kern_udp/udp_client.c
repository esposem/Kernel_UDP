#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>

#include "kclient_operations.h"

//############## CLIENT IP AND PORT (this module) #######
static char * name = "k_client";
static char print_name[SIZE_NAME];
module_param(name, charp, S_IRUGO);
MODULE_PARM_DESC(name,"The module name");

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

static unsigned long ns = 1;
module_param(ns,long, S_IRUGO);
MODULE_PARM_DESC(ns,"Delay between each send call (Throughput mode only)");

static unsigned long tsec = -1;
module_param(tsec, ulong, S_IRUGO);
MODULE_PARM_DESC(tsec,"How long the client should send messages (Throughput mode only)");
//######################################################

udp_service * udp_client;

static void connection_handler(void) {
  struct sockaddr_in dest_addr;

  init_default_messages();
  message_data * rcv_buff = create_message(0, 1);
  fill_sockaddr_in(&dest_addr, serverip, AF_INET, destport);

  switch(operation){
    case LATENCY:
      latency(rcv_buff, request, reply, &dest_addr);
      break;
    case TROUGHPUT:
      troughput(request, &dest_addr, ns, tsec);
      break;
    default:
      print(rcv_buff, request, reply, &dest_addr);
      break;
  }

  delete_message(rcv_buff);
  del_default_messages();
}

static int client_listen(void) {
  connection_handler();
  k_thread_stop(udp_client);
  return 0;
}

static void client_start(void){
  prepare_file(operation);
  init_service(&udp_client, print_name, ipmy, myport, client_listen, NULL);
}

static int __init client_init(void) {
  check_params(ipmy, myip, margs);
  check_params(serverip, destip, sargs);
  check_operation(&operation, opt);
  adjust_name(print_name, name, SIZE_NAME);
  printk(KERN_INFO "%s Destination is %d.%d.%d.%d:%d\n",print_name, serverip[0],serverip[1],serverip[2],serverip[3], destport);
  printk(KERN_INFO "%s opt: %c, ns: %lu, tsec: %ld\n",print_name, opt[0], ns, tsec);
  client_start();
  return 0;
}

static void __exit client_exit(void) {
  quit_service(udp_client);
  if(operation == TROUGHPUT){
    printk(KERN_INFO "%s Sent total of %llu packets\n",print_name, sent);
  }
}

module_init(client_init)
module_exit(client_exit)
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
