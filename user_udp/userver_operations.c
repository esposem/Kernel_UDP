#include "userver_operations.h"
#include <stdio.h>
#include <stdlib.h>

void echo_server(message_data *rcv_buf, message_data *rcv_check)
{
  int bytes_received, bytes_sent;
  struct msghdr reply, hdr;
  struct sockaddr_in dest;
  struct iovec iov[1];

  construct_header(&reply, NULL);
  construct_header(&hdr, &dest);

  char *recv_data = get_message_data(rcv_buf),
       *check_data = get_message_data(rcv_check);

  size_t check_size = get_message_size(rcv_check),
         check_total_s = get_total_mess_size(rcv_check),
         recv_size = get_total_mess_size(rcv_buf);

  if (recv_size < check_total_s)
  {
    printf("Server: Error, receiving buffer size is smaller than expected message\n");
    return;
  }

  printf("Server: waiting for clients...\n");

  while (stop)
  {

    fill_hdr(&hdr, iov, rcv_buf, recv_size);
    bytes_received = recvmsg(udps_socket, &hdr, MSG_WAITALL);

    if (bytes_received == check_total_s && memcmp(recv_data, check_data, check_size) == 0)
    {
      reply.msg_name = (struct sockaddr_in *)hdr.msg_name;
      fill_hdr(&reply, iov, rcv_buf, bytes_received);
      if ((bytes_sent = sendmsg(udps_socket, &reply, 0)) != bytes_received)
      {
        printf("Server: Error %d in sending: server not active\n", bytes_sent);
      }
    }
  }
}