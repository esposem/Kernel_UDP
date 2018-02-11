#include "kclient_operations.h"
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

unsigned long long sent = 0;

static struct hrtimer hr_timer;
enum hrtimer_restart res;
ktime_t ktime;
int stop = 1;


enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer )
{
  // hrtimer_forward(timer, ktime_get() , ktime);
  stop = 0;
  printk(KERN_INFO "STOP!\n");
  // return res;
  return HRTIMER_NORESTART;
}

void troughput(message_data * send_buf, struct sockaddr_in * dest_addr, unsigned long ns_int){

  struct common_data timep;
  struct timespec * old_timep = &timep.old_time, \
                  * current_timep = &timep.current_time, \
                  * temp;

  unsigned long diff_time;
  unsigned long long seconds=0, sent_sec=0;
  int seconds_counter = 0;
  int bytes_sent, interval_counter = 0;
  // ns_int *= 1000;
  timep.size_avg = sizeof(timep.average);
  timep.average[0] = '0';
  timep.average[1] = '\0';

  char * send_data = send_buf->mess_data;
  size_t send_size = send_buf->mess_len;

  struct msghdr hdr;
  construct_header(&hdr, dest_addr);

  res = HRTIMER_RESTART;
  ktime = ktime_set( 20, 0);
  hrtimer_init( &hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
  hr_timer.function = &my_hrtimer_callback;
  hrtimer_start( &hr_timer, ktime, HRTIMER_MODE_REL );

  printk("%s Throughput test: this module will count how many packets it sends\n", udp_client->name);
  getrawmonotonic(old_timep);

  while(stop){

    if(kthread_should_stop() || signal_pending(current)){
      break;
      // return;
    }

    getrawmonotonic(current_timep);
    diff_time = (current_timep->tv_sec - old_timep->tv_sec)*_1_SEC_TO_NS + current_timep->tv_nsec - old_timep->tv_nsec;

    interval_counter+= (int) diff_time;

    if(interval_counter >= ns_int){
      if((bytes_sent = udp_send(udpc_socket, &hdr, send_data, send_size)) == send_size){
        sent_sec++;
      }else{
        printk(KERN_ERR "%s Error %d in sending: server not active\n", udp_client->name, bytes_sent);
      }
      interval_counter = 0;
    }

    seconds_counter+= (int) diff_time;

    if(seconds_counter >= _1_SEC_TO_NS){
      seconds++;
      sent+= sent_sec;
      division(sent,seconds, timep.average, timep.size_avg);
      printk("C: Sent:%lld/sec   Avg:%s   Tot:%llu\n", sent_sec, timep.average, sent);
      sent_sec = 0;
      seconds_counter = 0;
    }

    temp = current_timep;
    current_timep = old_timep;
    old_timep = temp;
  }
  res = HRTIMER_NORESTART;
  hrtimer_cancel( &hr_timer );
  sent += sent_sec;
}


void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr){

  struct common_data timep;
  unsigned long long total_latency = 0, correctly_received = 0;
  int bytes_received, bytes_sent, loop_closed = 1, time_interval = _1_SEC_TO_NS - ABS_ERROR, one_sec = 0;
  unsigned long diff_time;

  char * send_data = send_buf->mess_data, \
       * recv_data = rcv_buf->mess_data, \
       * check_data = rcv_check->mess_data;

  size_t check_size = rcv_check->mess_len, \
         send_size = send_buf->mess_len, \
         recv_size = rcv_buf->mess_len;

  if(recv_size < check_size){
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message", udp_client->name);
    return;
  }

  timep.size_avg = sizeof(timep.average);
  timep.average[0] = '0';
  timep.average[1] = '\0';

  struct msghdr hdr;
  construct_header(&hdr, dest_addr);

  printk("%s Latency test: this module will count how long it takes to send and receive a packet\n", udp_client->name);

  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      return;
    }

    if(loop_closed){
      getrawmonotonic(&timep.old_time);
      if((bytes_sent = udp_send(udpc_socket, &hdr, send_data, send_size)) != send_size){
        printk(KERN_ERR "%s Error %d in sending: server not active\n", udp_client->name, bytes_sent);
        continue;
      }
      loop_closed = 0;
    }

    // blocks at most for one second
    bytes_received = udp_receive(udpc_socket, &hdr, recv_data, recv_size);
    getrawmonotonic(&timep.current_time);
    diff_time = (timep.current_time.tv_sec - timep.old_time.tv_sec)*_1_SEC_TO_NS + timep.current_time.tv_nsec - timep.old_time.tv_nsec;

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data, check_data, check_size) == 0){
      total_latency += diff_time;
      one_sec += diff_time;
      correctly_received++;
      loop_closed = 1;
      division(total_latency,correctly_received, timep.average, timep.size_avg);
    }else if(bytes_received < 0){ // nothing received
      one_sec = _1_SEC_TO_NS;
    }

    if(one_sec >= time_interval){
      one_sec = 0;
      printk(KERN_INFO "%s Latency average is %s\n",udp_client->name, timep.average);
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
  printk(KERN_INFO "%s Received %s (size %d) from %pI4:%hu\n",udp_client->name, recv_data, bytes_received, &address->sin_addr, ntohs(address->sin_port));
  printk(KERN_INFO "%s All done, terminating client\n", udp_client->name);
}
