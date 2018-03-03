#include "kclient_operations.h"
#include "kernel_udp.h"
#include "k_file.h"

unsigned long long sent = 0;

static unsigned long diff_time(struct timespec * op1, struct timespec * op2){
  return (op1->tv_sec - op2->tv_sec)*_1_SEC_TO_NS + op1->tv_nsec - op2->tv_nsec;
}

#define SIZE_SAMPLE 1200
#define TIME_SAMPLE _1_SEC_TO_NS / 10 // 100 ms after how munch should it keep the sample
#define MAX_MESS_WAIT _1_SEC_TO_NS / 10 // max amount a message can wait

struct client{
  // id the client
  int id;
  // flag to check if it is able to send or not
  int busy;
  int send_test; // when not busy, send latency test
  // how much it's waiting for packet 0-1sec max
  long waiting;
  // keeps track of sample latency
  struct timespec start_lat;
  int time_valid;
  unsigned long long total_received;
};


void init_clients(struct client * cl, int n){
  for (int i = 0; i < n; i++) {
    cl[i].id = i;
    cl[i].busy = 0;
    cl[i].send_test = 0;
    cl[i].waiting = 0;
    cl[i].total_received = 0;
  }
}

static int sample_send(struct client * cl){
  return cl->send_test == 1;
}

static void string_write(char * str){
  file_write(f,0, str, strlen(str));
}

static void write_results(int nclients, struct client * cl, unsigned long * troughput, unsigned long * sample_lat){
  printk(KERN_INFO "%s Writing results into a file...\n", get_service_name(cl_thread_1));
  if(f != NULL){
    int size = sizeof(unsigned long long) + sizeof(unsigned long) + 50;
    char * data = kmalloc(size, GFP_KERNEL);

    snprintf(data, size, "# %u %d\n", SIZE_SAMPLE, nclients);
    string_write(data);
    char * str = "#troughput\tlatency\n";
    string_write(str);
    for (int i = 100; i < SIZE_SAMPLE-100; i++) {
      snprintf(data, size, "%lu\t%lu\n", troughput[i], sample_lat[i*nclients]);
      string_write(data);
    }
    str = "\n\n";
    string_write(str);

    file_close(f);
    kfree(data);
    printk("%s Done\n", get_service_name(cl_thread_1));
  }else{
    printk(KERN_ERR "File error\n");
  }
}


void client_simulation(message_data * rcv_buf, message_data * send_buf, struct sockaddr_in * dest_addr, unsigned int nclients){
  char *  recv_data = get_message_data(rcv_buf), \
       * send_data = get_message_data(send_buf);

  size_t recv_size = get_total_mess_size(rcv_buf), \
         send_size = get_message_size(send_buf), \
         size_mess = get_total_mess_size(send_buf);

  if(recv_size < size_mess){
    printk(KERN_ERR "%s Error, receiving buffer size is smaller than expected message\n", get_service_name(cl_thread_1));
    return;
  }

  struct socket * sock = get_service_sock(cl_thread_1);
  int bytes_received, bytes_sent, id;
  unsigned long diff = 0;
  struct msghdr hdr;
  struct client * cl = kmalloc(sizeof(struct client) * nclients, GFP_KERNEL);
  struct timespec init_second, current_time;
  struct timespec * init_secondp = &init_second, \
                  * current_timep = &current_time, \
                  * temp;

  int tot_elements = nclients*SIZE_SAMPLE;
  unsigned long * troughput = kmalloc(sizeof(unsigned long)*SIZE_SAMPLE, GFP_KERNEL);
  unsigned long * sample_lat = kmalloc(sizeof(unsigned long)*tot_elements, GFP_KERNEL);

  memset(sample_lat, 0, sizeof(unsigned long)*tot_elements);
  memset(troughput, 0, sizeof(unsigned long)*SIZE_SAMPLE);

  int trough_count=0, lat_count = 0;
  long  time_spent = 0;

  printk("%s Started simulation\n", get_service_name(cl_thread_1));

  init_clients(cl, nclients);
  construct_header(&hdr, dest_addr);
  getrawmonotonic(init_secondp);

  while (trough_count < SIZE_SAMPLE) {

    if(kthread_should_stop() || signal_pending(current)){
      break;
    }

    for(int i=0; i < nclients; i++){
      if (cl[i].busy)
        continue;
      set_message_id(send_buf, i);

      if((bytes_sent = udp_send(sock, &hdr, send_buf, size_mess)) != size_mess){
        printk(KERN_ERR "%s Error %d in sending: server not active\n", get_service_name(cl_thread_1), bytes_sent);
        continue;
      }

      if(sample_send(&cl[i])){
        getrawmonotonic(&cl[i].start_lat);
        cl[i].send_test = 0;
        cl[i].time_valid = 1;
      }

      cl[i].busy = 1;

    }

    bytes_received = udp_receive(sock, &hdr, rcv_buf, recv_size);
    getrawmonotonic(current_timep);

    // calculate how much time passed
    diff = diff_time(current_timep, init_secondp);

    time_spent+= diff;
    // check message has been received correctly
    if(bytes_received == size_mess &&
      memcmp(recv_data, send_data, send_size) == 0 &&
      (id = get_message_id(rcv_buf)) < nclients){

      // update client data
      cl[id].total_received++;

      // if sample, save the statistics
      if(cl[id].time_valid && lat_count < tot_elements){
        sample_lat[lat_count++] = diff_time(current_timep, &cl[id].start_lat);
        cl[id].time_valid = 0;
      }
      // undo the effect of updates
      cl[id].waiting = MAX_MESS_WAIT;
    }


    for (int i = 0; i < nclients; i++) {
      cl[i].waiting += diff;
      if(cl[i].waiting >= MAX_MESS_WAIT){
        cl[i].busy = 0;
        cl[i].waiting = 0;
        if(cl[id].time_valid){
          cl[id].time_valid = 0;
        }
      }
    }

    if(time_spent >= TIME_SAMPLE){
      for (int i = 0; i < nclients; i++) {
        troughput[trough_count] += cl[i].total_received;
        cl[i].total_received = 0;
        cl[i].send_test = 1;
        cl[id].time_valid = 0;
      }
      time_spent = 0;
      trough_count++;
    }

    temp = current_timep;
    current_timep = init_secondp;
    init_secondp = temp;
  }

  write_results(nclients, cl, troughput, sample_lat);

  kfree(sample_lat);
  kfree(troughput);
  kfree(cl);
}


void troughput(message_data * send_buf, struct sockaddr_in * dest_addr, unsigned long ns_int, long tsec){

  struct common_data timep;
  struct timespec * old_timep = &timep.old_time, \
                  * current_timep = &timep.current_time, \
                  * temp;

  unsigned long diff;
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
    diff = diff_time(current_timep, old_timep);

    if((bytes_sent = udp_send(get_service_sock(cl_thread_1), &hdr, send_data, send_size)) == send_size){
      sent_sec++;
      ndelay(ns_int); // WARNING! BUSY WAIT
    }else{
      printk(KERN_ERR "%s Error %d in sending: server not active\n", get_service_name(cl_thread_1), bytes_sent);
    }

    seconds_counter+= (int) diff;

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
  unsigned long diff;

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
    diff = diff_time(&timep.current_time, &timep.old_time);

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data, check_data, check_size) == 0){
      total_latency += diff;
      one_sec += diff;
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
