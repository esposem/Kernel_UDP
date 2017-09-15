#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/udp.h>
#include <asm/atomic.h>
#include <linux/time.h>
#include <net/sock.h>
#include "kernel_udp.h"

//############## SERVER IP AND PORT (this module) #######
static unsigned char ipmy[5] = {127,0,0,4,'\0'};
static unsigned int myip[5];//random port
static int margs;
module_param_array(myip, int, &margs, S_IRUGO);
MODULE_PARM_DESC(myip,"The server ip, default 127.0.0.4");

static int myport = 3000;
module_param(myport, int, S_IRUGO);
MODULE_PARM_DESC(myport,"The receiving port, default 3000");
//######################################################

static udp_service * udp_server;
static struct socket * udps_socket;

#if SPEED_TEST
unsigned long long total_packets;
atomic_t rcv_pkt;
struct timer_list timer;
struct timeval interval;

void count_sec(unsigned long a){
  printk(KERN_INFO "%s Received %d/sec",udp_server->name, atomic_read(&rcv_pkt));
  atomic_set(&rcv_pkt, 0);
  mod_timer(&timer, jiffies + timeval_to_jiffies(&interval));
}
#endif

int connection_handler(void *data)
{
  struct sockaddr_in address;
  struct socket *learner_socket = udps_socket;
  int ret;
  unsigned char * in_buf = kmalloc(MAX_UDP_SIZE, GFP_KERNEL);
  unsigned char * out_buf= kmalloc(strlen(OK) +1, GFP_KERNEL);
  #if SPEED_TEST
  atomic_set(&rcv_pkt, 0);
  total_packets=0;
  setup_timer( &timer,  count_sec, 0);
  interval = (struct timeval) {1,0};
  mod_timer(&timer, jiffies + timeval_to_jiffies(&interval));
  printk("%s Performing a speed test: this module will count how many packets \
  it receives", udp_server->name);
  #endif

  while (1){

    if(kthread_should_stop() || signal_pending(current)){
      check_sock_allocation(udp_server, learner_socket);
      kfree(in_buf);
      kfree(out_buf);
      return 0;
    }
    ret = udp_server_receive(learner_socket, &address, in_buf, MSG_WAITALL,udp_server);
    if(ret > 0){
      #if SPEED_TEST
      atomic_inc(&rcv_pkt);
      total_packets++;
      #else
      printk(KERN_INFO "Got a message, printing it: %s", in_buf);
      if(memcmp(in_buf, HELLO, strlen(HELLO)) == 0){
        printk(KERN_INFO "%s recognized HELLO", udp_server->name);
        // do something
      }
      memset(out_buf, '\0', strlen(OK)+1);
      memcpy(out_buf, OK, strlen(OK)+1);
      udp_server_send(learner_socket, &address,out_buf, strlen(OK)+1, MSG_WAITALL, udp_server->name);
      // return 0; // stop from looping
      #endif
    }
  }
  return 0;
}

int udp_server_listen(void)
{
  udp_server_init(udp_server, &udps_socket, ipmy, &myport);
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
    check_params(ipmy, myip, margs);
    init_service(udp_server, "Server:");
    udp_server_start();
  }
  return 0;

}

static void __exit network_server_exit(void)
{
  #if SPEED_TEST
  del_timer(&timer);
  printk(KERN_INFO "%s Received total of %llu packets", udp_server->name, total_packets);
  #endif
  udp_server_quit(udp_server, udps_socket);
}

module_init(network_server_init)
module_exit(network_server_exit)
MODULE_LICENSE("MIT");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
