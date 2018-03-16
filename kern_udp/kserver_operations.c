#include "kserver_operations.h"
#include "k_file.h"
#include "kernel_udp.h"
#include <linux/kthread.h>

#define VERBOSE 1

unsigned long long
diff_time(struct timespec* op1, struct timespec* op2)
{
  return (op1->tv_sec - op2->tv_sec) * 1000000 +
         (op1->tv_nsec - op2->tv_nsec) / 1000;
}

void
echo_server(message_data* rcv_buf, message_data* rcv_check)
{

  int                bytes_received, bytes_sent;
  long               msg_count = 0, elapsed = 0;
  struct msghdr      reply, hdr;
  struct timespec    init_second, current_time;
  struct sockaddr_in dest;
  struct socket*     sock = get_service_sock(udp_server);
  construct_header(&reply, NULL);
  construct_header(&hdr, &dest);

  char *recv_data = get_message_data(rcv_buf),
       *check_data = get_message_data(rcv_check);

  size_t check_size = get_message_size(rcv_check),
         check_total_s = get_total_mess_size(rcv_check),
         recv_size = get_total_mess_size(rcv_buf);

  if (recv_size < check_total_s) {
    printk(KERN_ERR
           "%s Error, receiving buffer size is smaller than expected message\n",
           get_service_name(udp_server));
    return;
  }
  printk("%s Simulation\n", get_service_name(udp_server));
  getrawmonotonic(&init_second);

  while (1) {

    if (kthread_should_stop() || signal_pending(current)) {
      break;
    }
    bytes_received = udp_receive(sock, &hdr, rcv_buf, recv_size);
    msg_count++;
    getrawmonotonic(&current_time);
    elapsed = diff_time(&current_time, &init_second);

    if (VERBOSE && msg_count % 50000 == 0) {
      printk(KERN_INFO "Received %.ld msgs/sec.\n\n",
             msg_count * 1000000 / (elapsed));
      init_second = current_time;
      msg_count = 0;
    }

    if (bytes_received == check_total_s &&
        memcmp(recv_data, check_data, check_size) == 0) {
      reply.msg_name = (struct sockaddr_in*)hdr.msg_name;
      if ((bytes_sent = udp_send(sock, &reply, rcv_buf, bytes_received)) !=
          bytes_received) {
        printk(KERN_ERR "%s Error %d in sending: server not active\n",
               get_service_name(udp_server), bytes_sent);
      }
    }
  }
}
