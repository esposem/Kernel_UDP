#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "kserver_operations.h"

//############## SERVER IP AND PORT (this module) #######
static char *name = "k_server";
static char print_name[SIZE_NAME];
module_param(name, charp, S_IRUGO);
MODULE_PARM_DESC(name, "The module name");

static unsigned char ipmy[5] = {127, 0, 0, 1, '\0'};
static char *myip = NULL;
module_param(myip, charp, S_IRUGO);
MODULE_PARM_DESC(myip, "The server ip, default 127.0.0.4");

static int myport = 4000;
module_param(myport, int, S_IRUGO);
MODULE_PARM_DESC(myport, "The receiving port, default 3000");
//######################################################

//############## Types of operation #######
static char *opt = "s";
static enum operations operation = SIMULATION;
module_param(opt, charp, S_IRUGO);
MODULE_PARM_DESC(opt, "P or p for HELLO-OK, T or t for Troughput, L or l for "
                      "Latency, S or s for Simulation");
//######################################################

udp_service *udp_server;

static void connection_handler(void) {
  init_default_messages();
  message_data *rcv_buff = create_rcv_message();

  switch (operation) {
  case LATENCY:
    latency(rcv_buff, reply, request);
    break;
  case TROUGHPUT:
    troughput(rcv_buff, request);
    break;
  case PRINT:
    print(rcv_buff, reply, request);
    break;
  default:
    server_simulation(rcv_buff, request);
    break;
  }

  delete_message(rcv_buff);
  del_default_messages();
}

static int server_listen(void) {
  connection_handler();
  k_thread_stop(udp_server);
  return 0;
}

static void server_start(void) {
  // prepare_file(operation, 0);
  init_service(&udp_server, print_name, ipmy, myport, server_listen, NULL);
}

static int __init server_init(void) {
  check_params(ipmy, myip);
  check_operation(&operation, opt);
  adjust_name(print_name, name, SIZE_NAME);
  printk(KERN_INFO "%s opt: %c\n", print_name, opt[0]);
  server_start();
  return 0;
}

static void __exit server_exit(void) {
  quit_service(udp_server);
  if (operation == TROUGHPUT) {
    printk(KERN_INFO "%s Received total of %llu packets\n", print_name,
           received);
  }
}

module_init(server_init) module_exit(server_exit) MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
