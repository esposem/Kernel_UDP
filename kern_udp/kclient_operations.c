#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include "kclient_operations.h"
#include "kernel_udp.h"
#include "k_file.h"

unsigned long long sent = 0;

#if 0
struct stats{
  long count;
  long sample_lat;
  unsigned long long tot_time;
};

// thread shared variables
static atomic64_t window = ATOMIC64_INIT(0);
static int window_size = 500000;
static unsigned long sleep_ns = 10000;
static struct timespec leaving_time, arrival_time;
static int size_sample = 1000;
static struct stats values[size_sample];
static int interval_sample = 100;
// ------------------------

unsigned long long diff_time(struct timespec * op1, struct timespec * op2){
  return (op1->tv_sec - op2->tv_sec)*_1_SEC_TO_NS + op1->tv_nsec - op2->tv_nsec;
}

struct data{
  struct sockaddr_in addr;
  message_data mess;
};


void send_thread(void * datas){
  struct data * m = (struct data *) datas;
  struct sockaddr_in * dest_addr = &m->addr;
  char * send_data = m->mess.mess_data;
  size_t send_size = m->mess.mess_len;
  long long curr_win;
  int inc_speed = 0;
  struct timespec seconds_time, start_time;
  unsigned long long diff_tim, total_sent = 0;
  int change_delay = 500, bytes_sent;
  long temp;

  struct msghdr hdr;
  construct_header(&hdr, dest_addr);

  getrawmonotonic(&start_time);

  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      break;
    }
    if(total_sent % interval_sample == 0){
      bytes_sent = udp_send(get_service_sock(udp_client), &hdr, send_data, send_size);
    }else{
      bytes_sent = udp_send(get_service_sock(udp_client), &hdr, send_data, send_size);
    }

    if(bytes_sent != send_size){
      printk(KERN_ERR "%s Error %d in sending: server not active\n", get_service_name(udp_client), bytes_sent);
      continue;
    }
    ndelay(sleep_ns);
    getrawmonotonic(&seconds_time);

    curr_win = atomic64_inc_return(&window);
    if(curr_win == window_size){
      atomic64_set(&window, 0);
      sleep_ns+= change_delay;
    }else if(curr_win < window_size){
      diff_tim = diff_time(&seconds_time, &start_time);
      temp = sleep_ns - change_delay;
      if(diff_tim > _1_SEC_TO_NS && temp >= 0){
        sleep_ns = temp;
        atomic64_set(&window, 0);
        getrawmonotonic(&start_time);
      }
    }
    total_sent++;
  }
}

void measure_performances(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr){
  struct common_data timep;
  struct timespec seconds_time;
  unsigned long long total_latency = 0, correctly_received = 0, diff_time;
  int bytes_received, bytes_sent, loop_closed = 1, time_interval = _1_SEC_TO_NS - ABS_ERROR, one_sec = 0;
  int sample_interval = 100;
  struct stats data[1200];
  int data_counter = 0;

  char * send_data = get_message_data(send_buf), \
       * recv_data = get_message_data(rcv_buf), \
       * check_data = get_message_data(rcv_check);

  size_t check_size = get_message_size(rcv_check), \
         send_size = get_message_size(send_buf), \
         recv_size = get_message_size(rcv_buf);

  if(recv_size < check_size){
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message\n", get_service_name(udp_client));
    return;
  }

  timep.size_avg = sizeof(timep.average);
  timep.average[0] = '0';
  timep.average[1] = '\0';

  struct msghdr hdr;
  construct_header(&hdr, dest_addr);

  struct data * d = kmalloc(sizeof(struct data), GFP_KERNEL);
  memcpy(&d->addr, dest_addr, sizeof(struct sockaddr_in));
  memcpy(&d->mess, send_buf, sizeof(message_data));

  struct task_struct * newthread = kthread_run((void *)send_thread, d, "Send!");

  printk("%s Latency test: this module will count how long it takes to send and receive a packet\n", get_service_name(udp_client));
  getrawmonotonic(&seconds_time);

  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      if(kthread_stop(newthread) == 0){
        printk(KERN_INFO "%s Terminated thread\n", get_service_name(udp_client));
      }else{
        printk(KERN_INFO "%s Error in terminating thread\n", get_service_name(udp_client));
      }
      kfree(d);
      return;
    }

    // blocks at most for one second
    bytes_received = udp_receive(get_service_sock(udp_client), &hdr, recv_data, recv_size);
    getrawmonotonic(&timep.current_time);

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data, check_data, check_size) == 0){
      correctly_received++;
      if((correctly_received % sample_interval) == 0){
        data[data_counter].count = sample_interval;

        diff_time = (timep.current_time.tv_sec - timep.old_time.tv_sec)*_1_SEC_TO_NS + timep.current_time.tv_nsec - timep.old_time.tv_nsec;
        data[data_counter].sample_lat = diff_time;

        diff_time = (timep.current_time.tv_sec - seconds_time.tv_sec)*_1_SEC_TO_NS + timep.current_time.tv_nsec - seconds_time.tv_nsec;
        data[data_counter].tot_time = diff_time;
        data_counter++;
      }
      // loop_closed = 1;
      division(total_latency,correctly_received, timep.average, timep.size_avg);
    }else if(bytes_received < 0){ // nothing received
      one_sec = _1_SEC_TO_NS;
    }


    if(one_sec >= time_interval){
      one_sec = 0;
      printk(KERN_INFO "%s Latency average is %s\n",get_service_name(udp_client), timep.average);
    }

  }
  // printk(KERN_INFO "%s Writing statistics into file...\n",get_service_name(udp_client));

  // unsigned long mean = total_latency / sample_interval;
  // unsigned long sum_diff_2 = 0;
  // for (size_t i = 0; i < sample_interval; i++) {
  //   values[i] -= mean;
  //   values[i] *= values[i];
  //   sum_diff_2 += values[i];
  // }
  // unsigned long variance = sum_diff_2 / sample_interval;
  // unsigned long std = sqrt(variance);

  // if(f != NULL){
  //   char arr[100];
  //   memset(arr, '\0', 100);
  //   file_read(f, 0, arr, 20);
  //   printk(KERN_INFO "KERN Read %s\n",arr);
  //   char * str = "oleeeeee";
  //   file_write(f,0, str, strlen(str));
  //   file_close(f);
  //   printk("HELLO\n");
  // }


  //remove first and last 100
}

#endif

void troughput(message_data * send_buf, struct sockaddr_in * dest_addr, unsigned long ns_int, long tsec){

  struct common_data timep;
  struct timespec * old_timep = &timep.old_time, \
                  * current_timep = &timep.current_time, \
                  * temp;

  unsigned long diff_time;
  unsigned long long seconds=0, sent_sec=0;
  int seconds_counter = 0;
  int bytes_sent;
  timep.size_avg = sizeof(timep.average);
  timep.average[0] = '0';
  timep.average[1] = '\0';

  char * send_data = get_message_data(send_buf);
  size_t send_size = get_message_size(send_buf);

  struct msghdr hdr;
  construct_header(&hdr, dest_addr);

  printk("%s Throughput test: this module will count how many packets it sends\n", get_service_name(udp_client));
  getrawmonotonic(old_timep);

  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      break;
    }

    getrawmonotonic(current_timep);
    diff_time = (current_timep->tv_sec - old_timep->tv_sec)*_1_SEC_TO_NS + current_timep->tv_nsec - old_timep->tv_nsec;

    if((bytes_sent = udp_send(get_service_sock(udp_client), &hdr, send_data, send_size)) == send_size){
      sent_sec++;
      ndelay(ns_int);
    }else{
      printk(KERN_ERR "%s Error %d in sending: server not active\n", get_service_name(udp_client), bytes_sent);
    }

    seconds_counter+= (int) diff_time;

    if(seconds_counter >= _1_SEC_TO_NS){
      seconds++;
      sent+= sent_sec;
      division(sent,seconds, timep.average, timep.size_avg);
      printk("C: Sent:%lld/sec   Avg:%s   Tot:%llu\n", sent_sec, timep.average, sent);
      sent_sec = 0;
      seconds_counter = 0;
      if(seconds == tsec){
        printk(KERN_INFO "STOP!\n");
        break;
      }
    }

    temp = current_timep;
    current_timep = old_timep;
    old_timep = temp;
  }
  sent += sent_sec;
}

void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr){

  struct common_data timep;
  unsigned long long total_latency = 0, correctly_received = 0;
  int bytes_received, bytes_sent, loop_closed = 1, time_interval = _1_SEC_TO_NS - ABS_ERROR, one_sec = 0;
  unsigned long diff_time;

  char * send_data = get_message_data(send_buf), \
       * recv_data = get_message_data(rcv_buf), \
       * check_data = get_message_data(rcv_check);

  size_t check_size = get_message_size(rcv_check), \
         send_size = get_message_size(send_buf), \
         recv_size = get_message_size(rcv_buf);

  if(recv_size < check_size){
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message\n", get_service_name(udp_client));
    return;
  }

  timep.size_avg = sizeof(timep.average);
  timep.average[0] = '0';
  timep.average[1] = '\0';

  struct msghdr hdr;
  construct_header(&hdr, dest_addr);

  printk("%s Latency test: this module will count how long it takes to send and receive a packet\n", get_service_name(udp_client));

  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      return;
    }

    if(loop_closed){
      getrawmonotonic(&timep.old_time);
      if((bytes_sent = udp_send(get_service_sock(udp_client), &hdr, send_data, send_size)) != send_size){
        printk(KERN_ERR "%s Error %d in sending: server not active\n", get_service_name(udp_client), bytes_sent);
        continue;
      }
      loop_closed = 0;
    }

    // blocks at most for one second
    bytes_received = udp_receive(get_service_sock(udp_client), &hdr, recv_data, recv_size);
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
      printk(KERN_INFO "%s Latency average is %s\n",get_service_name(udp_client), timep.average);
    }
  }
}

void print(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr){

  int bytes_received, bytes_sent;
  struct sockaddr_in * address;

  char * send_data = get_message_data(send_buf), \
       * recv_data = get_message_data(rcv_buf), \
       *  check_data = get_message_data(rcv_check);

  size_t check_size = get_message_size(rcv_check), \
         send_size = get_message_size(send_buf), \
         recv_size = get_message_size(rcv_buf);

  struct msghdr hdr;
  construct_header(&hdr, dest_addr);

  if(recv_size < check_size){
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message\n", get_service_name(udp_client));
    return;
  }

  printk("%s Performing a simple test: this module will send OK and will wait to receive %s from server\n", get_service_name(udp_client), send_data);

  if((bytes_sent = udp_send(get_service_sock(udp_client), &hdr, send_data, send_size)) == send_size){
    address = hdr.msg_name;
    printk(KERN_INFO "%s Sent %s (size %zu) to %pI4 : %hu\n",get_service_name(udp_client), send_data, send_size, &address->sin_addr, ntohs(address->sin_port));
  }else{
    printk(KERN_ERR "%s Error %d in sending: server not active\n", get_service_name(udp_client), bytes_sent);
  }

  do{

    if(kthread_should_stop() || signal_pending(current)){
      return;
    }
    bytes_received = udp_receive(get_service_sock(udp_client), &hdr, recv_data, recv_size);

  }while(!(bytes_received == MAX_MESS_SIZE && memcmp(recv_data, check_data, check_size) == 0));

  address = hdr.msg_name;
  printk(KERN_INFO "%s Received %s (size %d) from %pI4:%hu\n",get_service_name(udp_client), recv_data, bytes_received, &address->sin_addr, ntohs(address->sin_port));
  printk(KERN_INFO "%s All done, terminating client\n", get_service_name(udp_client));
}
