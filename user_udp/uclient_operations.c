#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include "uclient_operations.h"
#include "user_udp.h"

unsigned long long sent =0;

static unsigned long diff_time(struct timespec * op1, struct timespec * op2){
  return (op1->tv_sec - op2->tv_sec)*_1_SEC_TO_NS + op1->tv_nsec - op2->tv_nsec;
}

#define SIZE_SAMPLE 1200
#define TIME_SAMPLE _1_SEC_TO_NS / 10 // 100 ms after how munch should it keep the sample
#define MAX_MESS_WAIT _1_SEC_TO_NS / 10 // max amount a message can wait
#define DISCARD_INIT 100
#define DISCARD_END 100

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

struct statistic{
  unsigned long * data;
  int count;
};

static void init_stat(struct statistic * s, size_t size){
  s->count = 0;
  s->data = malloc(sizeof(unsigned long)*size);
  memset(s->data,0,sizeof(unsigned long)*size);
}

static void del_stat(struct statistic * s){
  free(s->data);
}

void init_clients(struct client * cl, int n){
  for (int i = 0; i < n; i++) {
    cl[i].id = i;
    cl[i].busy = 0;
    cl[i].send_test = 0;
    cl[i].waiting = 0;
    cl[i].time_valid = 0;
    cl[i].total_received = 0;
  }
}

static int sample_send(struct client * cl){
  return cl->send_test == 1;
}

static int string_write(char * str, unsigned int nfile){
  return write(f[nfile], str, strlen(str));
}

static void write_results(int nclients, struct client * cl, struct statistic * troughput, struct statistic * sample_lat, unsigned int nfile){
  printf("Client: Writing results into a file...\n");
  if(f[nfile] < 0){
    printf("File error\n");
    return;
  }

  int size = sizeof(unsigned long long) + sizeof(unsigned long) + 50;
  char * data = malloc(size);

  snprintf(data, size, "# %u %d\n", SIZE_SAMPLE-DISCARD_INIT-DISCARD_END, nclients);
  string_write(data, nfile);
  char * str = "#troughput\tlatency\n";
  string_write(str, nfile);
  for (int i = DISCARD_INIT; i < SIZE_SAMPLE-DISCARD_END; i++) {
    snprintf(data, size, "%lu\t%lu\n", troughput->data[i], sample_lat->data[i]);
    string_write(data, nfile);
  }
  str = "\n\n";
  string_write(str, nfile);

  close(f[nfile]);
  f[nfile] = -1;
  free(data);
  printf("Client: Done\n");

}


void client_simulation(message_data * rcv_buf, message_data * send_buf, struct sockaddr_in * dest_addr, unsigned int nclients, unsigned int nfile){
  char *  recv_data = get_message_data(rcv_buf), \
       * send_data = get_message_data(send_buf);

  size_t recv_size = get_total_mess_size(rcv_buf), \
         send_size = get_message_size(send_buf), \
         size_mess = get_total_mess_size(send_buf);

  if(recv_size < size_mess){
  printf("Client: Error, receiving buffer size is smaller than expected message\n");
  return;
  }

  int bytes_received, bytes_sent, id;
  unsigned long diff = 0;
  struct msghdr hdr;
  struct client * cl = malloc(sizeof(struct client) * nclients);
  struct timespec init_second, current_time;
  struct timespec * init_secondp = &init_second, \
                  * current_timep = &current_time, \
                  * temp;

  struct statistic troughput;
  struct statistic sample_lat;
  init_stat(&troughput, SIZE_SAMPLE);
  init_stat(&sample_lat, SIZE_SAMPLE);

  int lat_pick = 0;
  long  time_spent = 0;

  printf("Client: Started simulation with %u clients\n", nclients);

  init_clients(cl, nclients);

  struct iovec iov[1];
  construct_header(&hdr, dest_addr);
  clock_gettime(CLOCK_MONOTONIC_RAW, init_secondp);

  while (troughput.count < SIZE_SAMPLE && stop) {

    for(int i=0; i < nclients; i++){
      if (cl[i].busy)
        continue;
      set_message_id(send_buf, i);

      fill_hdr(&hdr, iov, send_buf, size_mess);
      if(sendmsg(udpc_socket,&hdr, 0) != size_mess) {
        perror("ERROR IN SENDTO");
        continue;
      }

      if(sample_send(&cl[i])){
        clock_gettime(CLOCK_MONOTONIC_RAW, &cl[i].start_lat);
        cl[i].send_test = 0;
        cl[i].time_valid = 1;
      }

      cl[i].busy = 1;

    }

    fill_hdr(&hdr, iov, rcv_buf, recv_size);
    bytes_received = recvmsg(udpc_socket, &hdr, MSG_WAITALL);
    clock_gettime(CLOCK_MONOTONIC_RAW, current_timep);

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
      if(cl[id].time_valid){
        sample_lat.data[sample_lat.count++] = diff_time(current_timep, &cl[id].start_lat);
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
        if(cl[i].time_valid){
          sample_lat.data[sample_lat.count++] = ULONG_MAX;
          cl[i].time_valid = 0;
        }
      }
    }

    if(time_spent >= TIME_SAMPLE){
      for (int i = 0; i < nclients; i++) {
        troughput.data[troughput.count] += cl[i].total_received;
        cl[i].total_received = 0;
        // cl[i].time_valid = 0;
      }
      cl[lat_pick % nclients].send_test = 1;
      lat_pick++;
      time_spent = 0;
      troughput.count++;
    }

    temp = current_timep;
    current_timep = init_secondp;
    init_secondp = temp;
  }

  write_results(nclients, cl, &troughput, &sample_lat, nfile);

  del_stat(&sample_lat);
  del_stat(&troughput);
  free(cl);
}


void troughput(message_data * send_buf, struct sockaddr_in * dest_addr, unsigned long ns_int, long tsec){
  struct timespec old_time, current_time;
  struct timespec * old_timep = &old_time, * current_timep = &current_time, * temp;
  struct timespec sl = {0, ns_int};

  unsigned long diff;
  unsigned long long seconds = 0, sent_sec = 0;
  int seconds_counter = 0;
  int bytes_sent;
  double average;

  char * send_data = get_message_data(send_buf);
  size_t send_size = get_message_size(send_buf);

  struct msghdr hdr;
  struct iovec iov[1];
  construct_header(&hdr, dest_addr);

  printf("Client: Throughput test: this module will count how many packets it sends\n");
  clock_gettime(CLOCK_MONOTONIC_RAW, old_timep);

  while(stop){

    clock_gettime(CLOCK_MONOTONIC_RAW, current_timep);
    diff = diff_time(current_timep, old_timep);

    fill_hdr(&hdr, iov, send_data,send_size);
    if((bytes_sent = sendmsg(udpc_socket, &hdr, 0)) == send_size){
      sent_sec++;
      nanosleep(&sl,NULL);
    }else{
      printf("Client: Error %d in sending: server not active\n", bytes_sent);
    }


    seconds_counter+= (int) diff;

    if(seconds_counter >= _1_SEC_TO_NS){
      seconds++;
      sent+=sent_sec;
      average = (double)sent/seconds;
      printf("C: Sent:%lld/sec   Avg:%.3f   Tot:%llu\n", sent_sec, average, sent);
      sent_sec = 0;
      seconds_counter = 0;
      if(seconds == tsec){
        printf("STOP!\n");
        break;
      }
    }

    temp = current_timep;
    current_timep = old_timep;
    old_timep = temp;
  }

  sent +=sent_sec;
}

void latency(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr){

  double average;
  struct timespec departure_time, arrival_time;
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
   printf("Client: Error, receiving buffer size is smaller than expected message\n");
   return;
  }

  struct msghdr hdr;
  struct iovec iov[1];
  construct_header(&hdr, dest_addr);

  printf("Client: Latency test: this module will count how long it takes to send and receive a packet\n");

  while(stop){

    if(loop_closed){
      clock_gettime(CLOCK_MONOTONIC_RAW, &departure_time);
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
    clock_gettime(CLOCK_MONOTONIC_RAW, &arrival_time);
    diff = diff_time(&arrival_time, &departure_time);

    if(bytes_received == MAX_MESS_SIZE && memcmp(recv_data, check_data, check_size) == 0){
      total_latency += diff;
      one_sec += diff;
      correctly_received++;
      loop_closed = 1;
      average = (double)total_latency /correctly_received;
    }else if(bytes_received < 0){ // nothing received
      one_sec = _1_SEC_TO_NS;
    }

    if(one_sec >= time_interval){
      one_sec = 0;
      printf("Client: Latency average is %.3f\n", average);
    }

  }
}

void print(message_data * rcv_buf, message_data * send_buf, message_data * rcv_check, struct sockaddr_in * dest_addr){
  int bytes_received;

  char * send_data = get_message_data(send_buf), \
       * recv_data = get_message_data(rcv_buf), \
       *  check_data = get_message_data(rcv_check);

  size_t check_size = get_message_size(rcv_check), \
         send_size = get_message_size(send_buf), \
         recv_size = get_message_size(rcv_buf);

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
