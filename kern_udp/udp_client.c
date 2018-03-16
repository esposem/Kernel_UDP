#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "k_file.h"
#include "kclient_operations.h"

//############## CLIENT IP AND PORT (this module) #######
static char* name = "k_client";
static char  print_name[SIZE_NAME];
module_param(name, charp, S_IRUGO);
MODULE_PARM_DESC(name, "The module name");

static unsigned char ipmy[5] = { 127, 0, 0, 1, '\0' };
static char*         myip = NULL;
module_param(myip, charp, S_IRUGO);
MODULE_PARM_DESC(myip, "The client ip, default 127.0.0.1");

static int myport = 4000; // 0 random port
module_param(myport, int, S_IRUGO);
MODULE_PARM_DESC(
  myport, "The receiving port, default is a free random one chosen by system");
//######################################################

//############## SERVER IP AND PORT (udp_server) #######
static unsigned char serverip[5] = { 127, 0, 0, 1, '\0' };
static char*         destip = NULL;
module_param(destip, charp, S_IRUGO);
MODULE_PARM_DESC(destip, "The server ip, default 127.0.0.1");

static int destport = 3000;
module_param(destport, int, S_IRUGO);
MODULE_PARM_DESC(destport, "The server port, default 3000");
//######################################################

//############## Number of outstanding messages (clients) #######
static unsigned int nclients = 1;
module_param(nclients, uint, S_IRUGO);
MODULE_PARM_DESC(nclients, "The number of clients to simulate");
//######################################################

//############## Duration of the experiment #######
static unsigned int duration = 30;
module_param(duration, uint, S_IRUGO);
MODULE_PARM_DESC(duration, "Duration of the experiment");
//######################################################

udp_service* cl_thread_1;

static int
run_client(void* arg)
{
  struct sockaddr_in dest_addr;
  message_data*      rcv_buff;
  struct file*       f = arg;

  init_default_messages();
  rcv_buff = create_rcv_message();
  fill_sockaddr_in(&dest_addr, serverip, AF_INET, destport);
  run_outstanding_clients(rcv_buff, request, &dest_addr, nclients, duration, f);
  printk("Simulation ended\n");
  delete_message(rcv_buff);
  del_default_messages();
  k_thread_stop(cl_thread_1);
  return 0;
}

static int __init
           client_init(void)
{
  char         filen[256];
  struct file* f;

  check_params(ipmy, myip);
  check_params(serverip, destip);
  adjust_name(print_name, name, SIZE_NAME);
  printk(KERN_INFO "%s Destination is %d.%d.%d.%d:%d\n", print_name,
         serverip[0], serverip[1], serverip[2], serverip[3], destport);
  printk(KERN_INFO "%s, clients: %u, duration: %u\n", print_name, nclients,
         duration);
  sprintf(filen, "./results/kernel_data/results-%u.txt", nclients);
  f = file_open(filen, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
  init_service(&cl_thread_1, print_name, ipmy, myport, run_client, f);
  return 0;
}

static void __exit
            client_exit(void)
{
  quit_service(cl_thread_1);
}

module_init(client_init) module_exit(client_exit) MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
