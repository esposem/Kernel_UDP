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

#define MODULE_NAME "UDP Client"
#define MAX_RCV_WAIT 100000 // in microseconds

static int port = 3000;
module_param(port, int, S_IRUGO);
MODULE_PARM_DESC(port,"Server port, default 3000");

static char * destip = "127.0.0.1";
module_param(destip, charp, S_IRUGO);
MODULE_PARM_DESC(destip,"Server ip, default 127.0.0.1");

static int len = 49;
module_param(len, int, S_IRUGO);
MODULE_PARM_DESC(len,"Packet length, default 49 (automatically added space for \0)");

struct udp_client_service{
  struct socket * client_socket;
  struct task_struct * client_thread;
};

static struct udp_client_service * udp_client;
static atomic_t released_socket = ATOMIC_INIT(0);  // 0 no, 1 yes
static atomic_t thread_running = ATOMIC_INIT(0);   // 0 no, 1 yes
static atomic_t struct_allocated = ATOMIC_INIT(0); // 0 no, 1 yes
static char ** ipadd;
static unsigned char realip[5] = {127,0,0,1,'\0'};

int freeall(int n){
  n--;
  while (n >= 0){
    kfree(ipadd[n]);
    n--;
  }
  kfree(ipadd);
  return 0;
}

int isValidIp4 (char *str) {
  int segs = 0;
  int chcnt = 0;
  int accum = 0;
  int firstiszero = 0;
  int size_str = 0;
  char * begin = str;
  int num = 0;

  if (str == NULL){
    return freeall(num);
  }


  size_str = strlen(str);
  if(memcmp(str, "localhost", size_str > 9 ? 9 : size_str) == 0){
    freeall(0);
    return 2;
  }

  while (*str != '\0') {
    // printk(KERN_INFO "%c", *str);

    if(num > 3){
      return freeall(4);
    }

    if (*str == '.') {

      if (chcnt == 0)
        return freeall(num);

      if (++segs == 4)
        return freeall(num);

      ipadd[num] = kmalloc(str-begin + 1,GFP_KERNEL);
      memcpy(ipadd[num], begin, str-begin);
      ipadd[num][str-begin] = '\0';
      num++;

      chcnt = accum = 0;
      firstiszero = 0;
      str++;
      continue;
    }

    if ((*str < '0') || (*str > '9'))
      return freeall(num);

    if(chcnt == 0 && *str == '0'){
      // printk(KERN_INFO "it' s a beginning zero");
      firstiszero = 1;
    } else if(chcnt == 1 && firstiszero == 1){
      // printk(KERN_INFO "number after zero? free(%d)", num);
      return freeall(num);
    }else{
      // printk(KERN_INFO "firstiszero = 0");
      firstiszero = 0;
    }

    if ((accum = accum * 10 + *str - '0') > 255)
      return freeall(num);

    if(chcnt == 0){
      begin = str;
    }

    chcnt++;
    str++;
  }
  if (segs != 3)
    return freeall(num);

  if (chcnt == 0)
    return freeall(num);

  ipadd[num] = kmalloc(str-begin + 1,GFP_KERNEL);
  memcpy(ipadd[num], begin, str-begin);
  ipadd[num][str-begin] = '\0';

  return 1;
}

u32 create_address(u8 *ip)
{
  u32 addr = 0;
  int i;

  for(i=0; i<4; i++)
  {
    addr += ip[i];
    if(i==3)
    break;
    addr <<= 8;
  }
  return addr;
}

int udp_client_send(struct socket *sock, struct sockaddr_in * address, const char *buf, const size_t length, unsigned long flags)
{
  struct msghdr msg;
  struct kvec vec;
  int len, written = 0, left = length;
  mm_segment_t oldmm;

  msg.msg_name    = address;
  msg.msg_namelen = sizeof(struct sockaddr_in);
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags   = flags;

  oldmm = get_fs(); set_fs(KERNEL_DS);
  repeat_send:
  vec.iov_len = left;
  vec.iov_base = (char *)buf + written;

  // if(kthread_should_stop() || signal_pending(current)){
  //   printk(KERN_INFO MODULE_NAME": STOP [udp_client_send]");
  //   if(atomic_read(&released_socket) == 0){
  //     printk(KERN_INFO MODULE_NAME": Released socket [udp_client_send]");
  //     atomic_set(&released_socket, 1);
  //     sock_release(udp_client->client_socket);
  //   }
  //   return 0;
  // }

  len = kernel_sendmsg(sock, &msg, &vec, left, left);
  if((len == -ERESTARTSYS) || (!(flags & MSG_WAITALL) && (len == -EAGAIN)))
    goto repeat_send;

  if(len > 0){
    written += len;
    left -= len;
    if(left)
    goto repeat_send;
  }

  set_fs(oldmm);
  return written ? written:len;
}


int udp_client_receive(struct socket *sock, struct sockaddr_in *address, unsigned char *buf,int size, unsigned long flags)
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

    if (kthread_should_stop() || signal_pending(current)){
      // printk(KERN_INFO MODULE_NAME": STOP [udp_client_receive]");
      if(atomic_read(&released_socket) == 0){
        printk(KERN_INFO MODULE_NAME": Released socket [udp_client_receive]");
        atomic_set(&released_socket, 1);
        sock_release(udp_client->client_socket);
      }
      return 0;
    }else{
      len = kernel_recvmsg(sock, &msg, &vec, size, size, flags);
      if(len > 0){
        struct sockaddr_in * sa = (struct sockaddr_in *) msg.msg_name;
        printk(KERN_INFO MODULE_NAME": Received message %s from %pI4 (server) [udp_client_receive]", buf, &sa->sin_addr);
      }
    }
  }

  return len;
}

int udp_client_connect(void)
{
  struct sockaddr_in saddr;
  struct socket * conn_socket;
  // unsigned char destip[5] = {127,0,0,1,'\0'};

  char response[len+1];
  char reply[len+1];
  int ret = -1;
  struct timeval tv;

  printk(KERN_INFO MODULE_NAME": Server IP: %s [udp_client_connect]", destip);
  ret = sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &udp_client->client_socket);
  if(ret < 0){
    printk(KERN_INFO MODULE_NAME": Error: %d while creating socket [udp_client_connect]", ret);
    return -1;
  }else{
    printk(KERN_INFO MODULE_NAME": Created socket [udp_client_connect]");
    atomic_set(&released_socket, 0);
  }

  conn_socket = udp_client->client_socket;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  saddr.sin_addr.s_addr = htonl(create_address(realip));

  ret = conn_socket->ops->connect(conn_socket, (struct sockaddr *)&saddr, sizeof(saddr), O_RDWR);
  if(ret && (ret != -EINPROGRESS))
  {
    printk(KERN_INFO MODULE_NAME": Could not connect to server [udp_client_connect] %d", ret);
    atomic_set(&released_socket, 1);
    sock_release(udp_client->client_socket);
    return -1;
  }else{
    printk(KERN_INFO MODULE_NAME": Connected to server [udp_client_connect]");
  }

  tv.tv_sec = 0;
  tv.tv_usec = MAX_RCV_WAIT;
  kernel_setsockopt(conn_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));

  memset(&reply, 0, len+1);
  strcat(reply, "HELLO");
  printk(KERN_INFO MODULE_NAME": Sent HELLO to server [udp_client_connect]");
  udp_client_send(conn_socket, &saddr, reply, strlen(reply), MSG_WAITALL);

  printk(KERN_INFO MODULE_NAME": Waiting to receive a message [udp_client_connect]");
  while (1){

    if (kthread_should_stop() || signal_pending(current)){
      // printk(KERN_INFO MODULE_NAME": STOP [udp_client_connect]");
      if(atomic_read(&released_socket) == 0){
        printk(KERN_INFO MODULE_NAME": Released socket [udp_client_connect]");
        atomic_set(&released_socket, 1);
        sock_release(udp_client->client_socket);
      }
      return 0;
    }

    memset(response, 0, len+1);
    ret = udp_client_receive(conn_socket, &saddr, response, len,MSG_WAITALL);
    if(ret > 0){
      printk(KERN_INFO MODULE_NAME": Got %s [udp_client_connect]", response);

      // memset(&reply, 0, len+1);
      // strcat(reply, "OK");
      // printk(KERN_INFO MODULE_NAME": Sent OK to server [udp_client_connect]");
      // udp_client_send(conn_socket, &saddr, reply, strlen(reply), MSG_WAITALL);
    }
  }

  printk(KERN_INFO MODULE_NAME": Terminated conversation [udp_client_connect]");
  return 0;
}

void udp_client_start(void){
  udp_client->client_thread = kthread_run((void *) udp_client_connect, NULL, MODULE_NAME);
  if(udp_client->client_thread >= 0){
    atomic_set(&thread_running,1);
    printk(KERN_INFO MODULE_NAME ": Thread running [udp_client_start]");
  } else{
    printk(KERN_INFO MODULE_NAME ": Error in starting thread. Terminated [udp_client_start]");
  }
}

static int __init network_client_init(void)
{
  int i =0;
  ipadd = kmalloc(4 * sizeof(char *), GFP_KERNEL);
  if(ipadd){
    switch (isValidIp4(destip)) {
      case 1:
        // printk(KERN_INFO "Given valid ip address %s", destip);
        for(i=0; i < 4; i++){
          long l;
          kstrtol(ipadd[i], 10, &l);
          realip[i] = l;
        }
        freeall(4);
        break;
      // case 2:
        // printk(KERN_INFO "Given localhost");
        // break;
      // default:
        // printk(KERN_INFO "Given not valid address ip, using the default one");
    }
  }

  atomic_set(&released_socket, 1);
  udp_client = kmalloc(sizeof(struct udp_client_service), GFP_KERNEL);
  if(!udp_client){
    printk(KERN_INFO MODULE_NAME": Failed to initialize client [network_client_init]");
  }else{
    atomic_set(&struct_allocated,1);
    memset(udp_client, 0, sizeof(struct udp_client_service));
    printk(KERN_INFO MODULE_NAME": Client initialized [network_client_init]");
    udp_client_start();
  }
  return 0;
}

static void __exit network_client_exit(void)
{
  int ret;
  if(atomic_read(&struct_allocated) == 1){

    if(atomic_read(&thread_running) == 1){
      if((ret = kthread_stop(udp_client->client_thread)) == 0){
        printk(KERN_INFO MODULE_NAME": Terminated thread [network_client_exit]");
      }else{
        printk(KERN_INFO MODULE_NAME": Error %d in terminating thread [network_client_exit]", ret);
      }
    }else{
      printk(KERN_INFO MODULE_NAME": Thread was not running [network_client_exit]");
    }

    if(atomic_read(&released_socket) == 0){
      atomic_set(&released_socket, 1);
      sock_release(udp_client->client_socket);
      printk(KERN_INFO MODULE_NAME": Released socket [network_client_exit]");
    }

    kfree(udp_client);

  }else{
    printk(KERN_INFO MODULE_NAME": Struct was not allocated [network_client_exit]");
  }

  printk(KERN_INFO MODULE_NAME": Module unloaded [network_client_exit]");
}

module_init(network_client_init)
module_exit(network_client_exit)
