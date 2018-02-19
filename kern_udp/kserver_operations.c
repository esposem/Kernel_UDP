#include "kserver_operations.h"
#include "kernel_udp.h"
#include "k_file.h"

unsigned long long received = 0;

void troughput(message_data * rcv_buf, message_data * rcv_check){

  struct timespec departure_time, arrival_time;
  unsigned long long diff_time, rec_sec = 0, seconds = 0;
  int time_interval = _1_SEC_TO_NS - ABS_ERROR;

  char average[256];
  average[0] = '0';
  average[1] = '\0';

  int bytes_received;
  char * recv_data = get_message_data(rcv_buf), \
       * check_data = get_message_data(rcv_check);

  size_t check_size = get_message_size(rcv_check), \
         recv_size = get_message_size(rcv_buf), \
         size_tmval = sizeof(struct timespec),\
         size_avg = sizeof(average);

  if(recv_size < check_size){
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message\n", get_service_name(udp_server));
    return;
  }

  struct msghdr hdr;
  struct sockaddr_in dest;
  construct_header(&hdr, &dest);

  printk("%s Throughput test: this module will count how many packets it receives\n", get_service_name(udp_server));
  getrawmonotonic(&departure_time);

  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      break;
    }

    bytes_received = udp_receive(get_service_sock(udp_server),&hdr,recv_data, recv_size);
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
  received +=rec_sec;
}

void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check){

  int bytes_received, bytes_sent;
  struct msghdr reply, hdr;
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
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message\n", get_service_name(udp_server));
    return;
  }

  printk("%s Latency test\n", get_service_name(udp_server));


  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      break;
    }

    bytes_received = udp_receive(get_service_sock(udp_server),&hdr,recv_data, recv_size);

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data,check_data,check_size) == 0){
      reply.msg_name = (struct sockaddr_in *) hdr.msg_name;
      if((bytes_sent = udp_send(get_service_sock(udp_server), &reply, send_data, send_size)) != send_size){
        printk(KERN_ERR "%s Error %d in sending: server not active\n", get_service_name(udp_server), bytes_sent);
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

  char * send_data = get_message_data(send_buf), \
       * recv_data = get_message_data(rcv_buf), \
       * check_data = get_message_data(rcv_check);

  size_t check_size = get_message_size(rcv_check), \
         send_size = get_message_size(send_buf), \
         recv_size = get_message_size(rcv_buf);

  if(recv_size < check_size){
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message\n", get_service_name(udp_server));
    return;
  }

  printk("%s Performing a simple test: this module will wait to receive HELLO from client, replying with OK\n", get_service_name(udp_server));

  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      break;
    }

    bytes_received = udp_receive(get_service_sock(udp_server),&hdr,recv_data, recv_size);

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data,check_data,check_size) == 0){
      address = (struct sockaddr_in * ) hdr.msg_name;
      printk(KERN_INFO "%s Received %s (size %d) from %pI4:%hu\n",get_service_name(udp_server), recv_data, bytes_received, &address->sin_addr, ntohs(address->sin_port));
      reply.msg_name = address;
      if((bytes_sent = udp_send(get_service_sock(udp_server), &reply, send_data, send_size)) != send_size){
        printk(KERN_ERR "%s Error %d in sending: client not active\n", get_service_name(udp_server), bytes_sent);
      }else{
        printk(KERN_INFO "%s Sent %s (size %zu) to %pI4:%hu\n",get_service_name(udp_server), send_data, send_size, &address->sin_addr, ntohs(address->sin_port));
      }
    }
  }
}
