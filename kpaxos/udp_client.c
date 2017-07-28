#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <net/udp.h>
#include <asm/atomic.h>
#include <linux/time.h>
#include <net/sock.h>

#include "kernel_udp.h"

static unsigned char serverip[5] = {127,0,0,4,'\0'};
static int serverport = 3000;
static unsigned char myip[5] = {127,0,0,1,'\0'};
static int myport = 3001;

module_param(myport, int, S_IRUGO);
MODULE_PARM_DESC(myport,"The receiving port, default 3001");

static udp_service * udp_client;
static struct socket * udpc_socket;

int connection_handler(void *data)
{
  struct sockaddr_in address;
  struct socket *client_socket = udpc_socket;

  int ret;
  unsigned char * in_buf = kmalloc(MAX_UDP_SIZE, GFP_KERNEL);
  unsigned char * out_buf = kmalloc(strlen(HELLO)+1, GFP_KERNEL);

  address.sin_addr.s_addr = htonl(create_address(serverip));
  address.sin_family = AF_INET;
  address.sin_port = htons(serverport);

  memset(out_buf, '\0', strlen(HELLO)+1);
  memcpy(out_buf, HELLO, strlen(HELLO));
  
  udp_server_send(client_socket, &address, out_buf, strlen(out_buf)+1, MSG_WAITALL, udp_client->name);
  while (1){

    if(kthread_should_stop() || signal_pending(current)){
      check_sock_allocation(udp_client, client_socket);
      kfree(in_buf);
      kfree(out_buf);
      return 0;
    }

    ret = udp_server_receive(client_socket, &address, in_buf, MSG_WAITALL, udp_client);
    if(ret > 0){
      if(memcmp(in_buf, OK, strlen(OK)) == 0){
        printk(KERN_INFO "%s Got OK", udp_client->name);
        printk(KERN_INFO "%s All done, terminating client [connection_handler]", udp_client->name);
      }

    }
  }

  return 0;
}

int udp_server_listen(void)
{
  udp_server_init(udp_client, &udpc_socket, myip, &myport);
  connection_handler(NULL);
  atomic_set(&udp_client->thread_running, 0);
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
    init_service(udp_client, "Client:");
    udp_server_start();
  }
  return 0;
}

static void __exit network_server_exit(void)
{
  udp_server_quit(udp_client, udpc_socket);
}

module_init(network_server_init)
module_exit(network_server_exit)
MODULE_LICENSE("MIT");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
