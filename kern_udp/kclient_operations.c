#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include "kclient_operations.h"
#include "kernel_udp.h"
#include "k_file.h"

unsigned long long sent = 0;

unsigned long long diff_time(struct timespec * op1, struct timespec * op2){
  return (op1->tv_sec - op2->tv_sec)*_1_SEC_TO_NS + op1->tv_nsec - op2->tv_nsec;
}

#define SIZE_SAMPLE 1200
#define INTERVAL_SIZE 1000
int max_wait =  _1_SEC_TO_NS;

struct stats{
  long count;
  long sample_lat;
  unsigned long long tot_time;
};

struct client{
  // id the client
  int id;

  // flag to check if it is able to send or not
  int busy;
  // how much it's waiting for packet 0-1sec max
  long waiting;
  // keeps track of sample latency
  struct timespec start_lat;

  unsigned long long total_sent;
  unsigned long long total_received;
  // how long since its first send.
  // updated only if sample is received, or waiting expires
  unsigned long long life_time;
};

// TODO a buffer of ids of message to send to thread2, while thread1 receives
void init_clients(struct client * cl, int n){
  for (int i = 0; i < n; i++) {
    cl[i].id = i;
    cl[i].busy = 0;
    cl[i].total_sent = 0;
    cl[i].total_received = 0;
    cl[i].waiting = 0;
    cl[i].life_time = 0;
  }
}

void print_cl_info(struct client * cl, char * add){
  printk("%s Cl:%d busy:%d sent:%llu recv:%llu wait:%ld life:%llu\n", add, cl->id, cl->busy, cl->total_sent, cl->total_received, cl->waiting, cl->life_time);
}

int is_sample(struct client * cl){
  return (cl->total_sent % INTERVAL_SIZE) == 0;
}


void client_simulation(message_data * rcv_buf, message_data * send_buf, struct sockaddr_in * dest_addr){
  char *  recv_data = get_message_data(rcv_buf), \
       * send_data = get_message_data(send_buf);

  size_t recv_size = get_total_mess_size(rcv_buf), \
         send_size = get_message_size(send_buf), \
         size_mess = get_total_mess_size(send_buf); // size of all messages is always the same

  if(recv_size < size_mess){
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message\n", get_service_name(cl_thread_1));
    return;
  }

  struct socket * sock = get_service_sock(cl_thread_1);
  int bytes_received, bytes_sent, nclients = 10, id;
  long diff = 0;
  struct msghdr hdr;
  struct client cl[nclients];
  struct timespec init_second, current_time;
  struct timespec * init_secondp = &init_second, \
                  * current_timep = &current_time, \
                  * temp;

  struct stats * statistics = kmalloc(sizeof(struct stats)*nclients * SIZE_SAMPLE, GFP_KERNEL);
  memset(statistics, 0, sizeof(struct stats)*nclients * SIZE_SAMPLE);
  struct stats * s;
  int stat_count[nclients];
  memset(stat_count, 0, sizeof(stat_count));
  int full = 0;

  printk("%s Started simulation\n", get_service_name(cl_thread_1));

  init_clients(cl, nclients);
  construct_header(&hdr, dest_addr);
  getrawmonotonic(init_secondp);

  while (1) {

    if(kthread_should_stop() || signal_pending(current) || full == nclients){
      break;
    }

    for(int i=0; i < nclients;i++){
      if (cl[i].busy)
        continue;
      set_message_id(send_buf, i);

      if((bytes_sent = udp_send(sock, &hdr, send_buf, size_mess)) != size_mess){
        printk(KERN_ERR "%s Error %d in sending: server not active\n", get_service_name(cl_thread_1), bytes_sent);
        continue;
      }
      cl[i].busy = 1;
      cl[i].total_sent++;
      if(is_sample(&cl[i])){
        getrawmonotonic(&cl[i].start_lat);
      }
    }

    bytes_received = udp_receive(sock, &hdr, rcv_buf, recv_size);
    getrawmonotonic(current_timep);
    // check message has been received correctly
    if(bytes_received == size_mess &&
      memcmp(recv_data, send_data, send_size) == 0 &&
      (id = get_message_id(rcv_buf)) < nclients){

      // calculate how much time passed
      diff = diff_time(current_timep, init_secondp);
      // update client data
      cl[id].total_received++;
      cl[id].busy = 0;
      // update client life
      cl[id].life_time += cl[id].waiting + diff;
      // if sample, save the statistics
      if(is_sample(&cl[id]) && stat_count[id] < SIZE_SAMPLE){
        s = &statistics[id * SIZE_SAMPLE + stat_count[id]];
        s->count = INTERVAL_SIZE;
        s->sample_lat = diff_time(current_timep, &cl[id].start_lat);
        s->tot_time = cl[id].life_time;
        stat_count[id]++;
        if(stat_count[id] == SIZE_SAMPLE){
          full++;
        }
      }
      // undo the effect of updates
      cl[id].waiting = -diff;
    }else{
      diff = max_wait;
    }

    // increasing their waiting time and determines if they
    // should send a new message
    for (int i = 0; i < nclients; i++) {
      cl[i].waiting += diff;
      if(cl[i].waiting >= max_wait){
        cl[i].busy = 0;
        cl[i].life_time += cl[i].waiting;
        cl[i].waiting = 0;
        if(is_sample(&cl[i])){
          stat_count[i]++;
        }
      }
    }

    temp = current_timep;
    current_timep = init_secondp;
    init_secondp = temp;
  }

  #if 1
  printk(KERN_INFO "%s Writing results into a file...\n", get_service_name(cl_thread_1));
  if(f != NULL){

    file_write(f,0, "C\tcount\tsample\tabs_time\n", strlen("C\tcount\tsample\tabs_time\n"));

    for (int i = 0; i < nclients; i++) {
      char arr2[200];
      char res[100];
      division((cl[i].total_sent-cl[i].total_received)*100,cl[i].total_sent,res, 100);
      snprintf(arr2, 200, "Cl%d sent:%llu recv:%llu perc lost:%s\n", i, cl[i].total_sent, cl[i].total_received, res);
      file_write(f,0, arr2, strlen(arr2));
      for (int j = 1; j < SIZE_SAMPLE; j++) {
        char arr[200];
        snprintf(arr, 200, "%d\t%ld\t%ld\t%llu\n", i, statistics[i*SIZE_SAMPLE + j].count, statistics[i*SIZE_SAMPLE + j].sample_lat, statistics[i*SIZE_SAMPLE + j].tot_time - statistics[i*SIZE_SAMPLE + j -1].tot_time);
        file_write(f,0, arr, strlen(arr));
      }
    }
    file_close(f);
    printk("Done writing\n");
  }else{
    printk(KERN_ERR "File error\n");
  }
  #endif
  kfree(statistics);
}


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

  printk("%s Throughput test: this module will count how many packets it sends\n", get_service_name(cl_thread_1));
  getrawmonotonic(old_timep);

  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      break;
    }

    getrawmonotonic(current_timep);
    diff_time = (current_timep->tv_sec - old_timep->tv_sec)*_1_SEC_TO_NS + current_timep->tv_nsec - old_timep->tv_nsec;

    if((bytes_sent = udp_send(get_service_sock(cl_thread_1), &hdr, send_data, send_size)) == send_size){
      sent_sec++;
      ndelay(ns_int);
    }else{
      printk(KERN_ERR "%s Error %d in sending: server not active\n", get_service_name(cl_thread_1), bytes_sent);
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
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message\n", get_service_name(cl_thread_1));
    return;
  }

  timep.size_avg = sizeof(timep.average);
  timep.average[0] = '0';
  timep.average[1] = '\0';

  struct msghdr hdr;
  construct_header(&hdr, dest_addr);

  printk("%s Latency test: this module will count how long it takes to send and receive a packet\n", get_service_name(cl_thread_1));

  while(1){

    if(kthread_should_stop() || signal_pending(current)){
      return;
    }

    if(loop_closed){
      getrawmonotonic(&timep.old_time);
      if((bytes_sent = udp_send(get_service_sock(cl_thread_1), &hdr, send_data, send_size)) != send_size){
        printk(KERN_ERR "%s Error %d in sending: server not active\n", get_service_name(cl_thread_1), bytes_sent);
        continue;
      }
      loop_closed = 0;
    }

    // blocks at most for one second
    bytes_received = udp_receive(get_service_sock(cl_thread_1), &hdr, recv_data, recv_size);
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
      printk(KERN_INFO "%s Latency average is %s\n",get_service_name(cl_thread_1), timep.average);
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
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message\n", get_service_name(cl_thread_1));
    return;
  }

  printk("%s Performing a simple test: this module will send OK and will wait to receive %s from server\n", get_service_name(cl_thread_1), send_data);

  if((bytes_sent = udp_send(get_service_sock(cl_thread_1), &hdr, send_data, send_size)) == send_size){
    address = hdr.msg_name;
    printk(KERN_INFO "%s Sent %s (size %zu) to %pI4 : %hu\n",get_service_name(cl_thread_1), send_data, send_size, &address->sin_addr, ntohs(address->sin_port));
  }else{
    printk(KERN_ERR "%s Error %d in sending: server not active\n", get_service_name(cl_thread_1), bytes_sent);
  }

  do{

    if(kthread_should_stop() || signal_pending(current)){
      return;
    }
    bytes_received = udp_receive(get_service_sock(cl_thread_1), &hdr, recv_data, recv_size);

  }while(!(bytes_received == MAX_MESS_SIZE && memcmp(recv_data, check_data, check_size) == 0));

  address = hdr.msg_name;
  printk(KERN_INFO "%s Received %s (size %d) from %pI4:%hu\n",get_service_name(cl_thread_1), recv_data, bytes_received, &address->sin_addr, ntohs(address->sin_port));
  printk(KERN_INFO "%s All done, terminating client\n", get_service_name(cl_thread_1));
}
