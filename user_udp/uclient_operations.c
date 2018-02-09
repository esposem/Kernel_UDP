#include <stdio.h>
#include <stdlib.h>
#include "uclient_operations.h"

unsigned long long sent =0;

void troughput(message_data * send_buf, struct sockaddr_in * dest_addr, unsigned long us_int){
  struct timeval old_time, current_time;
  struct timeval * old_timep = &old_time, * current_timep = &current_time, * temp;

  int bytes_sent, interval_counter = 0, seconds_counter = 0;
  unsigned long long diff_time, sent_sec = 0, seconds = 0;

  double average;
  char * send_data = send_buf->mess_data;

  size_t send_size = send_buf->mess_len, \
          size_avg = sizeof(average);

  struct msghdr hdr;
  struct iovec iov[1];
  construct_header(&hdr, dest_addr);

  printf("Client: Throughput test: this module will count how many packets it sends\n");
  gettimeofday(old_timep, NULL);


  while(stop){

    gettimeofday(current_timep, NULL);
    diff_time = (current_timep->tv_sec - old_timep->tv_sec)*_1_SEC + current_timep->tv_usec - old_timep->tv_usec;

    interval_counter+= (int) diff_time;

    if(interval_counter >= us_int){
      fill_hdr(&hdr, iov, send_data,send_size);
      if((bytes_sent = sendmsg(udpc_socket, &hdr, 0)) == send_size){
        sent_sec++;
      }else{
        printf("Client: Error %d in sending: server not active\n", bytes_sent);
      }
      interval_counter = 0;
    }

    seconds_counter+= (int) diff_time;

    if(seconds_counter >= _1_SEC){
      seconds++;
      sent+=sent_sec;
      average = (double)sent/seconds;
      printf("Client: Sent %lld/sec, Average %.3f\n", sent_sec, average);
      sent_sec = 0;
      seconds_counter = 0;
    }

    temp = current_timep;
    current_timep = old_timep;
    old_timep = temp;
  }

  sent +=sent_sec;
  printf("Total number of sent packets: %llu\n", sent);
}

void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr){

  double average;
  struct timeval departure_time, arrival_time;
  unsigned long long total_latency = 0, correctly_received = 0;
  int bytes_received, loop_closed = 1, time_interval = _1_SEC - ABS_ERROR, \
  diff_time, one_sec = 0;

  char * send_data = send_buf->mess_data, \
       * recv_data = rcv_buf->mess_data, \
       * check_data = rcv_check->mess_data;

  size_t check_size = rcv_check->mess_len, \
         send_size = send_buf->mess_len, \
         recv_size = rcv_buf->mess_len;

  struct msghdr hdr;
  struct iovec iov[1];
  construct_header(&hdr, dest_addr);

  printf("Client: Latency test: this module will count how long it takes to send and receive a packet\n");

  while(stop){

    if(loop_closed){
      gettimeofday(&departure_time, NULL);
      fill_hdr(&hdr, iov, send_data,send_size);
      if(sendmsg(udpc_socket,&hdr, 0) != send_size) {
        perror("ERROR IN SENDTO");
        continue;
      }
      loop_closed = 0;
    }

    // blocks at most for one second
    fill_hdr(&hdr, iov, recv_data,recv_size);
    bytes_received = recvmsg(udpc_socket, &hdr, MSG_WAITALL);
    gettimeofday(&arrival_time, NULL);
    diff_time = (arrival_time.tv_sec - departure_time.tv_sec)*_1_SEC + arrival_time.tv_usec - departure_time.tv_usec;

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data, check_data, check_size) == 0){
      total_latency += diff_time;
      one_sec += diff_time;
      correctly_received++;
      loop_closed = 1;
      average = (double)total_latency /correctly_received;
    }else if(bytes_received < 0){ // nothing received
      one_sec = _1_SEC;
    }

    if(one_sec >= time_interval){
      one_sec = 0;
      printf("Client: Latency average is %.3f\n", average);
    }

  }
}

void print(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr){
  int bytes_received;

  char * send_data = send_buf->mess_data, \
       * recv_data = rcv_buf->mess_data, \
       * check_data = rcv_check->mess_data;

  size_t check_size = rcv_check->mess_len, \
         send_size = send_buf->mess_len, \
         recv_size = rcv_buf->mess_len;

  if(recv_size < check_size){
    printf("Client: Error, receiving buffer size is smaller than expected message\n");
    return;
  }
  printf("Client: Performing a simple test: this module will send %s and will wait to receive %s from server\n", send_data, "OK");


  struct msghdr hdr;
  struct iovec iov[1];
  construct_header(&hdr, dest_addr);

  fill_hdr(&hdr, iov, send_data,send_size);
  if(sendmsg(udpc_socket,&hdr, 0)<0) {
    perror("ERROR IN SENDTO");
    return;
  }
  printf("Client: sent %s (size %zu) to %s:%d \n", send_data, send_size, inet_ntoa(dest_addr->sin_addr), htons(dest_addr->sin_port));

  while(stop){
    fill_hdr(&hdr, iov, recv_data,recv_size);
    bytes_received = recvmsg(udpc_socket, &hdr, MSG_WAITALL);
    if((bytes_received == MAX_MESS_SIZE && memcmp(recv_data, check_data, check_size) == 0)){
        struct sockaddr_in * address = (struct sockaddr_in *) hdr.msg_name;
        printf("Client: Received %s (%d bytes) from %s:%d\n", recv_data, bytes_received, inet_ntoa(address->sin_addr), htons(address->sin_port));
      break;
    }
  }


}
