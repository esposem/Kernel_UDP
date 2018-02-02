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

#if TEST == 1
unsigned long long received = 0, res, seconds = 0, rec_min =0;
// atomic_t rec_min;
// struct timer_list timer;
// struct timeval interval;
struct timeval departure_time;
struct timeval arrival_time;

// void count_sec(unsigned long a){
//   printk(KERN_INFO "%s Received %d/sec",udp_server->name, atomic_read(&rec_min));
//   atomic_set(&rec_min, 0);
//   mod_timer(&timer, jiffies + timeval_to_jiffies(&interval));
// }
#endif

int connection_handler(void)
{
  struct sockaddr_in address;
  struct socket *learner_socket = udps_socket;
  int ret;
  unsigned char * in_buf = kmalloc(MAX_UDP_SIZE, GFP_KERNEL);
  unsigned char * out_buf = kmalloc(MAX_MESS_SIZE, GFP_KERNEL);
  memset(out_buf, '\0', MAX_MESS_SIZE);
  memcpy(out_buf, OK, strlen(OK)+1);

  #if TEST == 1
  // atomic_set(&rec_min, 0);
  // received=0;
  // setup_timer( &timer,  count_sec, 0);
  // interval = (struct timeval) {1,0};
  // mod_timer(&timer, jiffies + timeval_to_jiffies(&interval));
  do_gettimeofday(&departure_time);
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

    //in_buf gets cleaned inside
    ret = udp_server_receive(learner_socket, &address, in_buf, 0,udp_server);
    if(ret == MAX_MESS_SIZE && memcmp(in_buf, HELLO, MAX_MESS_SIZE) == 0){
      #if TEST != 1
        #if TEST == 0
          printk(KERN_INFO "%s Got a message: %s",udp_server->name, in_buf);
        #endif
        udp_server_send(learner_socket, &address,out_buf, MAX_MESS_SIZE, 0, udp_server->name);
        #if TEST == 0
          printk(KERN_INFO "%s Sent OK",udp_server->name);
        #endif

      #else
        // atomic_inc(&rec_min);
        // received++;
        rec_min++;
        do_gettimeofday(&arrival_time);
        res = (arrival_time.tv_sec * _100_MSEC + arrival_time.tv_usec) - (departure_time.tv_sec * _100_MSEC + departure_time.tv_usec );
        if(res >= _100_MSEC){
          seconds++;
          received +=rec_min;
          printk(KERN_INFO "%s Received %llu/sec",udp_server->name, rec_min);
          rec_min = 0;
          do_gettimeofday(&departure_time);
        }
      #endif
    }
  }
  return 0;
}

int server_listen(void)
{
  udp_server_init(udp_server, &udps_socket, ipmy, myport);
  if(atomic_read(&udp_server->thread_running) == 1){
    connection_handler();
    atomic_set(&udp_server->thread_running, 0);
  }
  return 0;
}

void server_start(void){
  udp_server->u_thread = kthread_run((void *)server_listen, NULL, udp_server->name);
  if(udp_server->u_thread >= 0){
    atomic_set(&udp_server->thread_running,1);
    printk(KERN_INFO "%s Thread running", udp_server->name);
  }else{
    printk(KERN_INFO "%s Error in starting thread. Terminated", udp_server->name);
  }
}

static int __init server_init(void)
{
  udp_server = kmalloc(sizeof(udp_service), GFP_KERNEL);
  if(!udp_server){
    printk(KERN_INFO "Failed to initialize server");
  }else{
    check_params(ipmy, myip, margs);
    init_service(udp_server, "Server:");
    server_start();
  }
  return 0;

}

static void __exit server_exit(void)
{
  #if TEST == 1
    // del_timer(&timer);
    printk(KERN_INFO "%s Received total of %llu packets", udp_server->name, received);
  #endif
  udp_server_quit(udp_server, udps_socket);
}

module_init(server_init)
module_exit(server_exit)
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
