#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <net/udp.h>
#include <asm/atomic.h>
#include <linux/time.h>
#include <net/sock.h>

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");

#define MODULE_NAME "UDP Server"
#define MAX_RCV_WAIT 100000 // in microseconds

static int port = 3000;
module_param(port, int, S_IRUGO);
MODULE_PARM_DESC(port,"The receiving port, default 3000");

static int len = 49;
module_param(len, int, S_IRUGO);
MODULE_PARM_DESC(len,"Packet length, default 49 (automatically added space for \0)");

struct udp_server_service
{
  struct socket * server_socket;
  struct task_struct * server_thread;
};

static struct udp_server_service * udp_server;
static atomic_t released_socket = ATOMIC_INIT(0); // 0 no, 1 yes
static atomic_t thread_running = ATOMIC_INIT(0);   // 0 no, 1 yes
static atomic_t struct_allocated = ATOMIC_INIT(0); // 0 no, 1 yes

int udp_server_send(struct socket *sock, struct sockaddr_in *address, const char *buf,\
  const size_t length, unsigned long flags)
  {
    struct msghdr msg;
    struct kvec vec;
    int len, written = 0, left =length;
    mm_segment_t oldmm;

    msg.msg_name    = address;
    msg.msg_namelen = sizeof(struct sockaddr_in);
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = flags;
    msg.msg_flags   = 0;

    oldmm = get_fs(); set_fs(KERNEL_DS);
    printk(KERN_INFO MODULE_NAME": Sent message to %pI4 [udp_server_send]", &address->sin_addr);

    repeat_send:

    vec.iov_len = left;
    vec.iov_base = (char *)buf + written;

    // if(kthread_should_stop() || signal_pending(current)){
    //   printk(KERN_INFO MODULE_NAME": STOP [udp_server_send]");
    //   if(atomic_read(&released_socket) == 0){
    //     printk(KERN_INFO MODULE_NAME": Released socket [udp_server_send]");
    //     atomic_set(&released_socket, 1);
    //     sock_release(udp_server->server_socket);
    //   }
    //   return 0;
    // }

    len = kernel_sendmsg(sock, &msg, &vec, left, left);

    if((len == -ERESTARTSYS) || (!(flags & MSG_WAITALL) && (len == -EAGAIN))){
      // printk(KERN_INFO MODULE_NAME"Sent only a piece [udp_server_send]");
      goto repeat_send;
    }

    if(len > 0)
    {
      written += len;
      left -= len;
      if(left){
        // printk(KERN_INFO MODULE_NAME"Sent only a piece [udp_server_send]" );
        goto repeat_send;
      }
    }

    set_fs(oldmm);
    return written?written:len;
  }

int udp_server_receive(struct socket *sock, struct sockaddr_in *address, unsigned char *buf,int size, unsigned long flags)
{
  struct msghdr msg;
  struct kvec vec;
  int len;

  msg.msg_name = address;
  msg.msg_namelen = sizeof(struct sockaddr_in);
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = flags;

  vec.iov_len = size;
  vec.iov_base = buf;

  len = -EAGAIN;
  while(len == -ERESTARTSYS || len == -EAGAIN){
    if(kthread_should_stop() || signal_pending(current)){
      // printk(KERN_INFO MODULE_NAME": STOP [udp_server_receive]");
      if(atomic_read(&released_socket) == 0){
        printk(KERN_INFO MODULE_NAME": Released socket [udp_server_receive]");
        atomic_set(&released_socket, 1);
        sock_release(udp_server->server_socket);
      }
      return 0;
    }else{
      len = kernel_recvmsg(sock, &msg, &vec, size, size, flags);
      if(len > 0){
        address = (struct sockaddr_in *) msg.msg_name;
        printk(KERN_INFO MODULE_NAME": Received message from %pI4 saying %s [udp_server_receive]",&address->sin_addr, buf);

      }
    }
  }

  return len;
}

int connection_handler(void *data)
{
  struct sockaddr_in address;
  struct socket *accept_socket = udp_server->server_socket;

  int ret;
  unsigned char in_buf[len+1];
  unsigned char out_buf[len+1];

  while (1){

    if(kthread_should_stop() || signal_pending(current)){
      // printk(KERN_INFO MODULE_NAME": STOP [connection_handler]");
      if(atomic_read(&released_socket) == 0){
        printk(KERN_INFO MODULE_NAME": Released socket [connection_handler]");
        atomic_set(&released_socket, 1);
        sock_release(udp_server->server_socket);
      }
      return 0;
    }

    memset(in_buf, 0, len+1);
    memset(&address, 0, sizeof(struct sockaddr_in));
    ret = udp_server_receive(accept_socket, &address, in_buf, len, MSG_WAITALL);
    if(ret > 0){
      printk(KERN_INFO MODULE_NAME": Got %s [connection_handler]", in_buf);
      memset(&out_buf, 0, len+1);
      strcat(out_buf, "GOT IT");
      udp_server_send(accept_socket, &address, out_buf,strlen(out_buf), MSG_WAITALL);
    }
  }

  return 0;
}

int udp_server_listen(void)
{
  int server_err;
  struct socket *conn_socket;
  struct sockaddr_in server;
  struct timeval tv;

  server_err = sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &udp_server->server_socket);
  if(server_err < 0){
    printk(KERN_INFO MODULE_NAME": Error: %d while creating socket [udp_server_listen]", server_err);
    return 0;
  }else{
    atomic_set(&released_socket, 0);
    printk(KERN_INFO MODULE_NAME": Created socket [udp_server_listen]");
  }

  conn_socket = udp_server->server_socket;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_family = AF_INET;
  server.sin_port = htons(port);

  server_err = conn_socket->ops->bind(conn_socket, (struct sockaddr*)&server, sizeof(server));
  if(server_err < 0) {
    printk(KERN_INFO MODULE_NAME": Error: %d while binding socket [udp_server_listen]", server_err);
    atomic_set(&released_socket, 1);
    sock_release(udp_server->server_socket);
    return 0;
  }else{
    printk(KERN_INFO MODULE_NAME": Socket is bind to any IP address of machine [udp_server_listen]");
  }

  tv.tv_sec = 0;
  tv.tv_usec = MAX_RCV_WAIT;
  kernel_setsockopt(conn_socket, SOL_SOCKET, SO_RCVTIMEO, (char * )&tv, sizeof(tv));

  connection_handler(NULL);
  return 0;
}

void udp_server_start(void){
  udp_server->server_thread = kthread_run((void *)udp_server_listen, NULL, MODULE_NAME);
  if(udp_server->server_thread >= 0){
    atomic_set(&thread_running,1);
    printk(KERN_INFO MODULE_NAME ": Thread running [udp_server_start]");
  }else{
    printk(KERN_INFO MODULE_NAME ": Error in starting thread. Terminated [udp_server_start]");
  }
}

static int __init network_server_init(void)
{
  atomic_set(&released_socket, 1);
  udp_server = kmalloc(sizeof(struct udp_server_service), GFP_KERNEL);
  if(!udp_server){
    printk(KERN_INFO MODULE_NAME": Failed to initialize server [network_server_init]");
  }else{
    atomic_set(&struct_allocated,1);
    memset(udp_server, 0, sizeof(struct udp_server_service));
    printk(KERN_INFO MODULE_NAME ": Server initialized [network_server_init]");
    udp_server_start();
  }
  return 0;
}

static void __exit network_server_exit(void)
{
  int ret;
  if(atomic_read(&struct_allocated) == 1){

    if(atomic_read(&thread_running) == 1){
      if((ret = kthread_stop(udp_server->server_thread)) == 0){
        printk(KERN_INFO MODULE_NAME": Terminated thread [network_server_exit]");
      }else{
        printk(KERN_INFO MODULE_NAME": Error %d in terminating thread [network_server_exit]", ret);
      }
    }else{
      printk(KERN_INFO MODULE_NAME": Thread was not running [network_server_exit]");
    }

    if(atomic_read(&released_socket) == 0){
      atomic_set(&released_socket, 1);
      sock_release(udp_server->server_socket);
      printk(KERN_INFO MODULE_NAME": Released socket [network_server_exit]");
    }

    kfree(udp_server);
  }else{
    printk(KERN_INFO MODULE_NAME": Struct was not allocated [network_server_exit]");
  }

  printk(KERN_INFO MODULE_NAME": Module unloaded [network_server_exit]");
}

module_init(network_server_init)
module_exit(network_server_exit)
