#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "kclient_operations.h"

//############## CLIENT IP AND PORT (this module) #######
static char * name = "k_client";
static char print_name[SIZE_NAME];
module_param(name, charp, S_IRUGO);
MODULE_PARM_DESC(name,"The module name");

static unsigned char ipmy[5] = {127,0,0,1,'\0'};
static char * myip = NULL;
module_param(myip, charp, S_IRUGO);
MODULE_PARM_DESC(myip,"The client ip, default 127.0.0.1");

static int myport = 3000; // 0 random port
module_param(myport, int, S_IRUGO);
MODULE_PARM_DESC(myport,"The receiving port, default is a free random one chosen by system");
//######################################################

//############## SERVER IP AND PORT (udp_server) #######
static unsigned char serverip[5] = {127,0,0,1,'\0'};
static char * destip = NULL;
module_param(destip, charp, S_IRUGO);
MODULE_PARM_DESC(destip,"The server ip, default 127.0.0.1");

static int destport = 4000;
module_param(destport, int, S_IRUGO);
MODULE_PARM_DESC(destport,"The server port, default 3000");
//######################################################

//############## Types of operation #######
static char * opt = "p";
static enum operations operation = PRINT;
module_param(opt, charp, S_IRUGO);
MODULE_PARM_DESC(opt,"P or p for HELLO-OK, T or t for Troughput, L or l for Latency, S or s for Simulation");

static unsigned long ns = 1;
module_param(ns,long, S_IRUGO);
MODULE_PARM_DESC(ns,"Delay between each send call (Throughput mode only)");

static unsigned long tsec = -1;
module_param(tsec, ulong, S_IRUGO);
MODULE_PARM_DESC(tsec,"How long the client should send messages (Throughput mode only)");

static unsigned int nclients = 0;
module_param(nclients, uint, S_IRUGO);
MODULE_PARM_DESC(nclients,"The number of clients to simulate");

static unsigned int ntests = 1;
module_param(ntests, uint, S_IRUGO);
MODULE_PARM_DESC(ntests,"The number of simulation to do (to the power of two)");

//######################################################

udp_service * cl_thread_1;
udp_service * cl_thread_2;

static void connection_handler(int thread_num) {
  struct sockaddr_in dest_addr;
  init_default_messages();

  message_data * rcv_buff = create_rcv_message();

  fill_sockaddr_in(&dest_addr, serverip, AF_INET, destport);
  int n = 1;

  switch(operation){
    case LATENCY:
      latency(rcv_buff, request, reply, &dest_addr);
      break;
    case TROUGHPUT:
      troughput(request, &dest_addr, ns, tsec);
      break;
    case PRINT:
      print(rcv_buff, request, reply, &dest_addr);
      break;
    default:
      if(nclients == 0){
        for (size_t i = 0; i < ntests; i++) {
          // n =  nclients, i = indexfile
          client_simulation(rcv_buff, request, &dest_addr, n, i);
          n*=2;
          ssleep(1);
        }
      }else{
        client_simulation(rcv_buff, request, &dest_addr, nclients, 0);
      }

      printk("Simulation ended\n");

      break;
  }

  delete_message(rcv_buff);
  del_default_messages();


}

static int client_receive(void) {
  connection_handler(1);
  k_thread_stop(cl_thread_1);
  return 0;
}

static void client_start(void){
  if(nclients == 0)
    prepare_files(operation, ntests);
  else
    prepare_files(operation, 1);
  init_service(&cl_thread_1, print_name, ipmy, myport, client_receive, NULL);
}

static int __init client_init(void) {
  check_params(ipmy, myip);
  check_params(serverip, destip);
  check_operation(&operation, opt);
  adjust_name(print_name, name, SIZE_NAME);
  printk(KERN_INFO "%s Destination is %d.%d.%d.%d:%d\n",print_name, serverip[0],serverip[1],serverip[2],serverip[3], destport);
  // printk(KERN_INFO "%s opt: %c, ns: %lu, tsec: %ld, nclients:%u, ntests:%u\n",print_name, opt[0], ns, tsec, nclients, ntests);
  printk(KERN_INFO "%s opt: %c, ns: %lu, tsec: %ld, ntests:%u\n",print_name, opt[0], ns, tsec, ntests);
  client_start();
  return 0;
}

static void __exit client_exit(void) {
  quit_service(cl_thread_1);
  if(operation == SIMULATION){
    if(nclients == 0){
      close_files(ntests);
    }else{
      close_files(1);
    }
  }

  if(operation == TROUGHPUT)
    printk(KERN_INFO "%s Sent total of %llu packets\n",print_name, sent);
}

module_init(client_init)
module_exit(client_exit)
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
