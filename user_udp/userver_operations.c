#include "userver_operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static unsigned long
diff_time(struct timespec* op1, struct timespec* op2)
{
  return (op1->tv_sec - op2->tv_sec) * 1e6 +
         (op1->tv_nsec - op2->tv_nsec) * 1e-3;
}

void
echo_server(message_data* rcv_buf, message_data* rcv_check, int verbose)
{
  int                bytes_received, bytes_sent;
  long               msg_count = 0, elapsed = 0;
  struct timespec    init_second, current_time;
  struct msghdr      reply, hdr;
  struct sockaddr_in dest;
  struct iovec       iov[1];
  char *             recv_data = get_message_data(rcv_buf),
       *check_data = get_message_data(rcv_check);
  size_t check_size = get_message_size(rcv_check),
         check_total_s = get_total_mess_size(rcv_check),
         recv_size = get_total_mess_size(rcv_buf);

  if (recv_size < check_total_s) {
    printf("Server: Error, receiving buffer size is smaller than expected "
           "message\n");
    return;
  }

  clock_gettime(CLOCK_MONOTONIC_RAW, &init_second);
  construct_header(&reply, NULL);
  construct_header(&hdr, &dest);
  fill_hdr(&hdr, iov, rcv_buf, recv_size);

  printf("Server: waiting for clients...\n");
  while (stop) {
    bytes_received = recvmsg(udps_socket, &hdr, MSG_WAITALL);
    msg_count++;
    clock_gettime(CLOCK_MONOTONIC_RAW, &current_time);
    elapsed = diff_time(&current_time, &init_second);
    if (verbose && msg_count % 50000 == 0) {
      printf("Received %.2lf msgs/sec.\n\n", msg_count / (elapsed * 1e-6));
      init_second = current_time;
      msg_count = 0;
    }

    if (bytes_received == check_total_s &&
        memcmp(recv_data, check_data, check_size) == 0) {
      reply.msg_name = (struct sockaddr_in*)hdr.msg_name;
      fill_hdr(&reply, iov, rcv_buf, bytes_received);
      if ((bytes_sent = sendmsg(udps_socket, &reply, 0)) != bytes_received)
        printf("Server: Error %d in sending: server not active\n", bytes_sent);
    }
  }
}