#include "kclient_operations.h"
#include <linux/kthread.h>

unsigned long long sent = 0;

void troughput(message_data * send_buf, struct sockaddr_in * dest_addr, unsigned long us_int){

  struct timeval old_time, current_time;
  struct timeval * old_timep = &old_time, * current_timep = &current_time, * temp;

  int bytes_sent, interval_counter = 0, seconds_counter = 0;
  unsigned long long diff_time, sent_sec = 0, seconds = 0;

  char average[256];
  average[0] = '0';
  average[1] = '\0';

  char * send_data = send_buf->mess_data;

  size_t send_size = send_buf->mess_len, \
          size_avg = sizeof(average);

  struct msghdr hdr;
  construct_header(&hdr, dest_addr);

  printk("%s Throughput test: this module will count how many packets it sends\n", udp_client->name);
  do_gettimeofday(old_timep);

  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      sent +=sent_sec;
      return;
    }

    do_gettimeofday(current_timep);
    diff_time = (current_timep->tv_sec - old_timep->tv_sec)*_1_SEC + current_timep->tv_usec - old_timep->tv_usec;

    interval_counter+= (int) diff_time;

    if(interval_counter >= us_int){
      if((bytes_sent = udp_send(udpc_socket, &hdr, send_data, send_size)) == send_size){
        sent_sec++;
      }else{
        printk(KERN_ERR "%s Error %d in sending: server not active\n", udp_client->name, bytes_sent);
      }
      interval_counter = 0;
    }

    seconds_counter+= (int) diff_time;

    if(seconds_counter >= _1_SEC){
      seconds++;
      sent+=sent_sec;
      division(sent,seconds, average, size_avg);
      printk("%s Sent %lld/sec, Average %s\n", udp_client->name, sent_sec, average);
      sent_sec = 0;
      seconds_counter = 0;
    }

    temp = current_timep;
    current_timep = old_timep;
    old_timep = temp;
  }
}


void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr){

  char average[256];
  struct timeval departure_time, arrival_time;
  unsigned long long total_latency = 0, correctly_received = 0;
  int bytes_received, bytes_sent, loop_closed = 1, time_interval = _1_SEC - ABS_ERROR, \
  diff_time, one_sec = 0;

  char * send_data = send_buf->mess_data, \
       * recv_data = rcv_buf->mess_data, \
       * check_data = rcv_check->mess_data;

  size_t check_size = rcv_check->mess_len, \
         send_size = send_buf->mess_len, \
         recv_size = rcv_buf->mess_len, \
         size_avg = sizeof(average);

  if(recv_size < check_size){
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message", udp_client->name);
    return;
  }

  average[0] = '0';
  average[1] = '\0';

  struct msghdr hdr;
  construct_header(&hdr, dest_addr);

  printk("%s Latency test: this module will count how long it takes to send and receive a packet\n", udp_client->name);

  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      return;
    }

    if(loop_closed){
      do_gettimeofday(&departure_time);
      if((bytes_sent = udp_send(udpc_socket, &hdr, send_data, send_size)) != send_size){
        printk(KERN_ERR "%s Error %d in sending: server not active\n", udp_client->name, bytes_sent);
        continue;
      }
      loop_closed = 0;
    }

    // blocks at most for one second
    bytes_received = udp_receive(udpc_socket, &hdr, recv_data, recv_size);
    do_gettimeofday(&arrival_time);
    diff_time = (arrival_time.tv_sec - departure_time.tv_sec)*_1_SEC + arrival_time.tv_usec - departure_time.tv_usec;

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data, check_data, check_size) == 0){
      total_latency += diff_time;
      one_sec += diff_time;
      correctly_received++;
      loop_closed = 1;
      division(total_latency,correctly_received, average, size_avg);
    }else if(bytes_received < 0){ // nothing received
      one_sec = _1_SEC;
    }

    if(one_sec >= time_interval){
      one_sec = 0;
      printk(KERN_INFO "%s Latency average is %s\n",udp_client->name, average);
    }
  }
}

void print(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr){

  int bytes_received, bytes_sent;
  struct sockaddr_in * address;

  char * send_data = send_buf->mess_data, \
       * recv_data = rcv_buf->mess_data, \
       *  check_data = rcv_check->mess_data;

  size_t check_size = rcv_check->mess_len, \
         send_size = send_buf->mess_len, \
         recv_size = rcv_buf->mess_len;

  struct msghdr hdr;
  construct_header(&hdr, dest_addr);

  if(recv_size < check_size){
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message", udp_client->name);
    return;
  }

  printk("%s Performing a simple test: this module will send %s and will wait to receive %s from server\n", udp_client->name, send_data, "OK");

  if((bytes_sent = udp_send(udpc_socket, &hdr, send_data, send_size)) == send_size){
    address = hdr.msg_name;
    printk(KERN_INFO "%s Sent %s (size %zu) to %pI4 : %hu",udp_client->name, send_data, send_size, &address->sin_addr, ntohs(address->sin_port));
  }else{
    printk(KERN_ERR "%s Error %d in sending: server not active\n", udp_client->name, bytes_sent);
  }

  do{

    if(kthread_should_stop() || signal_pending(current)){
      return;
    }
    bytes_received = udp_receive(udpc_socket, &hdr, recv_data, recv_size);

  }while(!(bytes_received == MAX_MESS_SIZE && memcmp(recv_data, check_data, check_size) == 0));

  address = hdr.msg_name;
  printk(KERN_INFO "%s Received %s (size %d) from %pI4:%hu",udp_client->name, recv_data, bytes_received, &address->sin_addr, ntohs(address->sin_port));
  printk(KERN_INFO "%s All done, terminating client", udp_client->name);
}
