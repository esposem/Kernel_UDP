#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/udp.h>
#include <asm/atomic.h>
#include <linux/time.h>
#include <net/sock.h>
#include "kernel_udp.h"

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
static unsigned char serverip[5] = {127,0,0,4,'\0'};
static unsigned int destip[5];//random port
static int sargs;
module_param_array(destip, int, &sargs, S_IRUGO);
MODULE_PARM_DESC(destip,"The server ip, default 127.0.0.1");

static int destport = 3000;
module_param(destport, int, S_IRUGO);
MODULE_PARM_DESC(destport,"The server port, default 3000");
//######################################################

static udp_service * udp_client;
static struct socket * udpc_socket;

#if SPEED_TEST
atomic_t sent_pkt;
struct timer_list timer;
struct timeval interval;
unsigned long long total_packets;

void count_sec(unsigned long a){
  printk(KERN_INFO "%s\t \t \t Sent %d/sec", udp_client->name, atomic_read(&sent_pkt));
  atomic_set(&sent_pkt, 0);
  mod_timer(&timer, jiffies + timeval_to_jiffies(&interval));
}
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/mm.h>

int connection_handler(void *data)
{
  struct sockaddr_in address;
  struct socket *client_socket = udpc_socket;


  unsigned char * in_buf = kmalloc(MAX_UDP_SIZE, GFP_KERNEL);
  unsigned char * out_buf = kmalloc(strlen(HELLO)+1, GFP_KERNEL);

  address.sin_addr.s_addr = htonl(create_address(serverip));
  address.sin_family = AF_INET;
  address.sin_port = htons(destport);

  memset(out_buf, '\0', strlen(HELLO)+1);
  memcpy(out_buf, HELLO, strlen(HELLO));

  #if SPEED_TEST
  atomic_set(&sent_pkt, 0);
  total_packets=0;
  setup_timer( &timer,  count_sec, 0);
  interval = (struct timeval) {1,0};
  mod_timer(&timer, jiffies + timeval_to_jiffies(&interval));
  printk("%s Performing a speed test: this module will count how many packets \
  it sends", udp_client->name);
  #else
  udp_server_send(client_socket, &address, out_buf, strlen(out_buf)+1, MSG_WAITALL, udp_client->name);
  #endif

  while (1){

    if(kthread_should_stop() || signal_pending(current)){
      check_sock_allocation(udp_client, client_socket);
      kfree(in_buf);
      kfree(out_buf);
      return 0;
    }

    #if SPEED_TEST
    udp_server_send(client_socket, &address, out_buf, strlen(out_buf)+1, MSG_WAITALL, udp_client->name);
    atomic_inc(&sent_pkt);
    total_packets++;
    #else
    int ret = udp_server_receive(client_socket, &address, in_buf, MSG_WAITALL, udp_client);
    if(ret > 0){
      if(memcmp(in_buf, OK, strlen(OK)) == 0){
        printk(KERN_INFO "%s Got OK", udp_client->name);
        printk(KERN_INFO "%s All done, terminating client [connection_handler]", udp_client->name);
      }
    }
    #endif


  }

  return 0;
}

int udp_server_listen(void)
{
  udp_server_init(udp_client, &udpc_socket, ipmy, &myport);
  if(atomic_read(&udp_client->thread_running) == 1){
    connection_handler(NULL);
    atomic_set(&udp_client->thread_running, 0);
  }
  return 0;
}

void udp_server_start(void){
  udp_client->u_thread = kthread_run((void *)udp_server_listen, NULL, udp_client->name);
  if(udp_client->u_thread >= 0){
    atomic_set(&udp_client->thread_running,1);
    printk(KERN_INFO "%s Thread running [udp_server_start]", udp_client->name);
  }else{
    printk(KERN_INFO "%s Error in starting thread. Terminated [udp_server_start]", udp_client->name);
  }
}

static int __init network_server_init(void)
{
  udp_client = kmalloc(sizeof(udp_service), GFP_KERNEL);
  if(!udp_client){
    printk(KERN_INFO "Failed to initialize CLIENT [network_server_init]");
  }else{
    check_params(ipmy, myip, margs);
    check_params(serverip, destip, sargs);
    init_service(udp_client, "Client:");
    udp_server_start();
  }
  return 0;
}

static void __exit network_server_exit(void)
{
  #if SPEED_TEST
  del_timer(&timer);
  printk(KERN_INFO "%s Sent total of %llu packets",udp_client->name, total_packets);
  #endif
  udp_server_quit(udp_client, udpc_socket);
}

module_init(network_server_init)
module_exit(network_server_exit)
MODULE_LICENSE("MIT");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
