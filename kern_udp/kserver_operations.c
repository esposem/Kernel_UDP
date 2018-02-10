#include "kserver_operations.h"
#include <linux/kthread.h>

unsigned long long received = 0;

void troughput(message_data * rcv_buf, message_data * rcv_check){

  struct timespec departure_time, arrival_time;
  unsigned long long diff_time, rec_sec = 0, seconds = 0;
  int time_interval = _1_SEC_TO_NS - ABS_ERROR;

  char average[256];
  average[0] = '0';
  average[1] = '\0';

  int bytes_received;
  char * recv_data = rcv_buf->mess_data, \
       * check_data = rcv_check->mess_data;

  size_t check_size = rcv_check->mess_len, \
         recv_size = rcv_buf->mess_len, \
         size_tmval = sizeof(struct timespec),\
         size_avg = sizeof(average);

  if(recv_size < check_size){
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message", udp_server->name);
    return;
  }

  struct msghdr hdr;
  struct sockaddr_in dest;
  construct_header(&hdr, &dest);

  printk("%s Throughput test: this module will count how many packets it receives\n", udp_server->name);
  getrawmonotonic(&departure_time);

  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      received +=rec_sec;
      break;
    }

    bytes_received = udp_receive(udps_socket,&hdr,recv_data, recv_size);
    getrawmonotonic(&arrival_time);

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data,check_data,check_size) == 0){
      rec_sec++;
    }

    diff_time = (arrival_time.tv_sec - departure_time.tv_sec)*_1_SEC_TO_NS + arrival_time.tv_nsec - departure_time.tv_nsec;
    if(diff_time >= time_interval){
      memcpy(&departure_time, &arrival_time, size_tmval);
      seconds++;
      received +=rec_sec;
      division(received,seconds, average, size_avg);
      printk(KERN_INFO "S: Recv:%lld/sec   Avg:%s   Tot:%llu\n", rec_sec, average, received);
      rec_sec = 0;
    }
  }
}

void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check){

  int bytes_received, bytes_sent;
  struct msghdr reply, hdr;
  struct sockaddr_in dest;
  construct_header(&reply, NULL);
  construct_header(&hdr, &dest);

  char * send_data = send_buf->mess_data, \
       * recv_data = rcv_buf->mess_data, \
       * check_data = rcv_check->mess_data;

  size_t check_size = rcv_check->mess_len, \
         send_size = send_buf->mess_len, \
         recv_size = rcv_buf->mess_len;

  if(recv_size < check_size){
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message", udp_server->name);
    return;
  }

  printk("%s Latency test\n", udp_server->name);


  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      break;
    }

    bytes_received = udp_receive(udps_socket,&hdr,recv_data, recv_size);

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data,check_data,check_size) == 0){
      reply.msg_name = (struct sockaddr_in *) hdr.msg_name;
      if((bytes_sent = udp_send(udps_socket, &reply, send_data, send_size)) != send_size){
        printk(KERN_ERR "%s Error %d in sending: server not active\n", udp_server->name, bytes_sent);
      }
    }
  }
}

void print(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check){

  int bytes_received, bytes_sent;
  struct sockaddr_in * address;
  struct sockaddr_in dest;

  struct msghdr reply, hdr;
  construct_header(&hdr, &dest);
  construct_header(&reply, NULL);

  char * send_data = send_buf->mess_data, \
       * recv_data = rcv_buf->mess_data, \
       * check_data = rcv_check->mess_data;

  size_t check_size = rcv_check->mess_len, \
         send_size = send_buf->mess_len, \
         recv_size = rcv_buf->mess_len;

  if(recv_size < check_size){
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message", udp_server->name);
    return;
  }

  printk("%s Performing a simple test: this module will wait to receive HELLO from client, replying with OK\n", udp_server->name);

  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      break;
    }

    bytes_received = udp_receive(udps_socket,&hdr,recv_data, recv_size);

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data,check_data,check_size) == 0){
      address = (struct sockaddr_in * ) hdr.msg_name;
      printk(KERN_INFO "%s Received HELLO (size %d) from %pI4:%hu",udp_server->name, bytes_received, &address->sin_addr, ntohs(address->sin_port));
      reply.msg_name = address;
      if((bytes_sent = udp_send(udps_socket, &reply, send_data, send_size)) != send_size){
        printk(KERN_ERR "%s Error %d in sending: client not active\n", udp_server->name, bytes_sent);
      }else{
        printk(KERN_INFO "%s Sent OK (size %zu) to %pI4 : %hu",udp_server->name, send_size, &address->sin_addr, ntohs(address->sin_port));
      }
    }
  }
}
