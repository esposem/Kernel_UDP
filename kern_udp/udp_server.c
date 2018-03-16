#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "kserver_operations.h"

//############## SERVER IP AND PORT (this module) #######
static char* name = "k_server";
static char  print_name[SIZE_NAME];
module_param(name, charp, S_IRUGO);
MODULE_PARM_DESC(name, "The module name");

static unsigned char ipmy[5] = { 127, 0, 0, 1, '\0' };
static char*         myip = NULL;
module_param(myip, charp, S_IRUGO);
MODULE_PARM_DESC(myip, "The server ip, default 127.0.0.4");

static int myport = 3000;
module_param(myport, int, S_IRUGO);
MODULE_PARM_DESC(myport, "The receiving port, default 3000");
//######################################################

udp_service* udp_server;

static int
server_listen(void* arg)
{
  init_default_messages();
  message_data* rcv_buff = create_rcv_message();
  echo_server(rcv_buff, request);
  delete_message(rcv_buff);
  del_default_messages();
  k_thread_stop(udp_server);
  return 0;
}

static int __init
           server_init(void)
{
  check_params(ipmy, myip);
  adjust_name(print_name, name, SIZE_NAME);
  init_service(&udp_server, print_name, ipmy, myport, server_listen, NULL);
  return 0;
}

static void __exit
            server_exit(void)
{
  quit_service(udp_server);
}

module_init(server_init);
module_exit(server_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
