#include <stdio.h>
#include <stdlib.h>
#include "userver_operations.h"

unsigned long long received = 0;

#define INTERVAL 1000
#define max_wait  _1_SEC_TO_NS / INTERVAL
#define NSEC 60
#define SSIZE INTERVAL * NSEC

unsigned long long diff_time(struct timespec * op1, struct timespec * op2){
  return (op1->tv_sec - op2->tv_sec)*_1_SEC_TO_NS + op1->tv_nsec - op2->tv_nsec;
}

void server_simulation(message_data * rcv_buf, message_data * rcv_check){

  int bytes_received, bytes_sent;
  struct msghdr reply, hdr;
  struct sockaddr_in dest;
  struct iovec iov[1];

  construct_header(&reply, NULL);
  construct_header(&hdr, &dest);

  char * recv_data = get_message_data(rcv_buf), \
       * check_data = get_message_data(rcv_check);

  size_t check_size = get_message_size(rcv_check), \
     check_total_s = get_total_mess_size(rcv_check), \
        recv_size = get_total_mess_size(rcv_buf);

  if(recv_size < check_total_s){
   printf("Server: Error, receiving buffer size is smaller than expected message\n");
   return;
  }

  printf("Server: Simulation\n");

  while(stop){

    fill_hdr(&hdr, iov, rcv_buf,recv_size);
    bytes_received = recvmsg(udps_socket, &hdr, MSG_WAITALL);

    if(bytes_received == check_total_s && memcmp(recv_data,check_data,check_size) == 0){
      reply.msg_name = (struct sockaddr_in *) hdr.msg_name;
      fill_hdr(&reply, iov, rcv_buf,bytes_received);
      if((bytes_sent = sendmsg(udps_socket, &reply, 0)) != bytes_received){
        printf("Server: Error %d in sending: server not active\n", bytes_sent);
      }
    }
  }
}

void troughput(message_data * rcv_buf, message_data * rcv_check){

  struct timeval departure_time, arrival_time;
  unsigned long long diff_time, rec_sec = 0, seconds = 0;
  int time_interval = _1_SEC_TO_NS - ABS_ERROR;

  double average;

  int bytes_received;
  char * recv_data = get_message_data(rcv_buf), \
         * check_data = get_message_data(rcv_check);

  size_t check_size = get_message_size(rcv_check), \
         recv_size = get_message_size(rcv_buf), \
         size_tmval = sizeof(struct timespec),\
         size_avg = sizeof(average);

  if(recv_size < check_size){
    printf("Server: Error, receiving buffer size is smaller than expected message\n");
    return;
  }

  struct msghdr hdr;
  struct iovec iov[1];
  struct sockaddr_in dest;
  construct_header(&hdr, &dest);

  printf("Server: Throughput test: this module will count how many packets it receives\n");
  gettimeofday(&departure_time, NULL);

  while(stop){

    fill_hdr(&hdr, iov, recv_data,recv_size);
    bytes_received = recvmsg(udps_socket, &hdr, MSG_WAITALL);
    gettimeofday(&arrival_time, NULL);

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data,check_data,check_size) == 0){
      rec_sec++;
    }

    diff_time = (arrival_time.tv_sec - departure_time.tv_sec)*_1_SEC_TO_NS + arrival_time.tv_usec - departure_time.tv_usec;
    if(diff_time >= time_interval){
      memcpy(&departure_time, &arrival_time, size_tmval);
      seconds++;
      received +=rec_sec;
      average = (double)received/seconds;
      printf("S: Recv:%lld/sec   Avg:%.3f   Tot:%llu\n", rec_sec, average, received);
      rec_sec = 0;
    }
  }
  received +=rec_sec;
  printf("Total number of received packets: %llu\n", received);
}

void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check){

  int bytes_received, bytes_sent;
  struct msghdr reply, hdr;
  struct iovec iov[1];
  struct sockaddr_in dest;
  construct_header(&reply, NULL);
  construct_header(&hdr, &dest);

  char * send_data = get_message_data(send_buf), \
       * recv_data = get_message_data(rcv_buf), \
       * check_data = get_message_data(rcv_check);

  size_t check_size = get_message_size(rcv_check), \
         send_size = get_message_size(send_buf), \
         recv_size = get_message_size(rcv_buf);

  if(recv_size < check_size){
    printf("Server: Error, receiving buffer size is smaller than expected message\n");
    return;
  }

  printf("Server: Latency test\n");


  while(stop){

    fill_hdr(&hdr, iov, recv_data,recv_size);
    bytes_received = recvmsg(udps_socket, &hdr, MSG_WAITALL);

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data,check_data,check_size) == 0){
      reply.msg_name = (struct sockaddr_in *) hdr.msg_name;
      fill_hdr(&reply, iov, send_data,send_size);
      if((bytes_sent = sendmsg(udps_socket, &reply, 0)) != send_size){
        printf("Server: Error %d in sending: server not active\n", bytes_sent);
      }
    }
  }
}

void print(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check){

  int bytes_received, bytes_sent;
  struct sockaddr_in * address;

  struct msghdr reply, hdr;
  struct iovec iov[1];
  struct sockaddr_in dest;
  construct_header(&hdr, &dest);
  construct_header(&reply, NULL);

  char * send_data = get_message_data(send_buf), \
       * recv_data = get_message_data(rcv_buf), \
       * check_data = get_message_data(rcv_check);

  size_t check_size = get_message_size(rcv_check), \
         send_size = get_message_size(send_buf), \
         recv_size = get_message_size(rcv_buf);

  if(recv_size < check_size){
    printf("Server: Error, receiving buffer size is smaller than expected message\n");
    return;
  }

  printf("Server: Performing a simple test: this module will wait to receive HELLO from client, replying with OK\n");
  while(stop){

    fill_hdr(&hdr, iov, recv_data,recv_size);
    bytes_received = recvmsg(udps_socket, &hdr, MSG_WAITALL);

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data,check_data,check_size) == 0){
      address = (struct sockaddr_in * ) hdr.msg_name;
      printf("Server: Received %s (size %d) from %s:%d\n", recv_data, bytes_received, inet_ntoa(address->sin_addr), ntohs(address->sin_port));
      reply.msg_name = address;
      fill_hdr(&reply, iov, send_data,send_size);
      if((bytes_sent = sendmsg(udps_socket, &reply, 0)) != send_size){
        printf("Server: Error %d in sending: client not active\n", bytes_sent);
      }else{
        printf("Server: Sent %s (size %zu) to %s:%d\n", send_data, send_size, inet_ntoa(address->sin_addr), ntohs(address->sin_port));
      }
    }
  }
}
