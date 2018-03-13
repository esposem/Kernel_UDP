#include "uclient_operations.h"
#include "user_udp.h"
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define SIZE_SAMPLE 10000
#define TIMEOUT_US 500 // max amount a message can wait in us
#define DISCARD_INIT 100
#define DISCARD_END 100

struct client
{
  int id;                    // id the client
  int busy;                  // flag to check if it is able to send or not
  long waiting;              // how much it's waiting for packet 0-1sec max
  struct timespec start_lat; // keeps track of sample latency
  unsigned long long total_received;
};

struct statistic
{
  unsigned long *data;
  int count;
};

unsigned long long sent = 0;

//time diff in microseconds
static unsigned long diff_time(struct timespec *op1, struct timespec *op2)
{
  return (op1->tv_sec - op2->tv_sec) * 10e6 + (op1->tv_nsec - op2->tv_nsec) / 10e3;
}

static void init_stat(struct statistic *s, size_t size)
{
  s->count = 0;
  s->data = malloc(sizeof(unsigned long) * size);
  memset(s->data, 0, sizeof(unsigned long) * size);
}

static void del_stat(struct statistic *s)
{
  free(s->data);
}

static void write_results(int nclients, struct client *cl, struct statistic *troughput, struct statistic *sample_lat)
{
  printf("Client: Writing results into a file...\n");
  char filen[100];
  sprintf(filen, "./results/user_data/results-process-%u.txt", getpid());

  int f = open(filen, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
  if (f < 0)
  {
    printf("File error\n");
    return;
  }

  char data[256];
  char *str = "#count\tlatency\n";

  sprintf(data, "# SMPL=%u, DSCD_I=%u, DSCD_E=%u, CL=%d\n", SIZE_SAMPLE, DISCARD_INIT, DISCARD_END, nclients);
  write(f, data, strlen(data));
  write(f, str, strlen(str));
  for (int i = DISCARD_INIT; i < SIZE_SAMPLE - DISCARD_END; i++)
  {
    sprintf(data, "%lu\t%lu\n", 1L, sample_lat->data[i]);
    write(f, data, strlen(data));
  }

  write(f, "\n\n", 2);
  close(f);
  free(data);
  printf("Client: Done\n");
}

void init_clients(struct client *cl, int n)
{
  memset(cl, 0, n * sizeof(struct client));
  for (int i = 0; i < n; i++)
    cl[i].id = i;
}

void run_outstanding_clients(message_data *rcv_buf, message_data *send_buf, struct sockaddr_in *dest_addr, unsigned int nclients, long duration)
{
  char *recv_data = get_message_data(rcv_buf),
       *send_data = get_message_data(send_buf);

  size_t recv_size = get_total_mess_size(rcv_buf),
         send_size = get_message_size(send_buf),
         size_mess = get_total_mess_size(send_buf);

  if (recv_size < size_mess)
  {
    printf("Client: Error, receiving buffer size is smaller than expected message\n");
    return;
  }

  int bytes_received, bytes_sent, id;
  unsigned long diff = 0;
  struct msghdr hdr;
  struct client *cl = malloc(sizeof(struct client) * nclients);
  struct timespec init_second, current_time;
  struct timespec *temp;

  struct statistic sample_lat;
  init_stat(&sample_lat, SIZE_SAMPLE);

  int lat_pick = 0;
  long time_spent = 0;

  printf("Client: Starting with %u clients\n", nclients);

  init_clients(cl, nclients);

  struct iovec iov[1];
  construct_header(&hdr, dest_addr);
  clock_gettime(CLOCK_MONOTONIC_RAW, &init_second);

  while (stop)
  {

    for (int i = 0; i < nclients; i++)
    {
      if (cl[i].busy)
        continue;

      clock_gettime(CLOCK_MONOTONIC_RAW, &cl[i].start_lat);
      set_message_id(send_buf, i);
      fill_hdr(&hdr, iov, send_buf, size_mess);
      if (sendmsg(udpc_socket, &hdr, 0) != size_mess)
      {
        perror("ERROR IN SENDTO");
        continue;
      }

      cl[i].busy = 1; // just sent a message
    }

    fill_hdr(&hdr, iov, rcv_buf, recv_size);
    bytes_received = recvmsg(udpc_socket, &hdr, MSG_WAITALL);
    clock_gettime(CLOCK_MONOTONIC_RAW, &current_time);

    // check message has been received correctly
    if (bytes_received == size_mess &&
        memcmp(recv_data, send_data, send_size) == 0 &&
        (id = get_message_id(rcv_buf)) < nclients)
    {

      // update client data
      cl[id].total_received++;
      sample_lat.count = sample_lat.count + 1 == SIZE_SAMPLE ? sample_lat.count : sample_lat.count + 1;
      sample_lat.data[sample_lat.count] = diff_time(&current_time, &cl[id].start_lat);
      cl[id].busy = 0;
    }

    for (int i = 0; i < nclients; i++)
      if (cl[i].busy && diff_time(&current_time, &cl[i].start_lat) > TIMEOUT_US)
        cl[i].busy = 0;
  }

  write_results(nclients, cl, NULL, &sample_lat);
  del_stat(&sample_lat);
  free(cl);
}

void print(message_data *rcv_buf, message_data *send_buf, message_data *rcv_check, struct sockaddr_in *dest_addr)
{
  int bytes_received;

  char *send_data = get_message_data(send_buf),
       *recv_data = get_message_data(rcv_buf),
       *check_data = get_message_data(rcv_check);

  size_t check_size = get_message_size(rcv_check),
         send_size = get_message_size(send_buf),
         recv_size = get_message_size(rcv_buf);

  if (recv_size < check_size)
  {
    printf("Client: Error, receiving buffer size is smaller than expected message\n");
    return;
  }
  printf("Client: Performing a simple test: this module will send %s and will wait to receive %s from server\n", send_data, "OK");

  struct msghdr hdr;
  struct iovec iov[1];
  construct_header(&hdr, dest_addr);

  fill_hdr(&hdr, iov, send_data, send_size);
  if (sendmsg(udpc_socket, &hdr, 0) < 0)
  {
    perror("ERROR IN SENDTO");
    return;
  }
  printf("Client: sent %s (size %zu) to %s:%d \n", send_data, send_size, inet_ntoa(dest_addr->sin_addr), htons(dest_addr->sin_port));

  while (stop)
  {
    fill_hdr(&hdr, iov, recv_data, recv_size);
    bytes_received = recvmsg(udpc_socket, &hdr, MSG_WAITALL);
    if ((bytes_received == MAX_MESS_SIZE && memcmp(recv_data, check_data, check_size) == 0))
    {
      struct sockaddr_in *address = (struct sockaddr_in *)hdr.msg_name;
      printf("Client: Received %s (%d bytes) from %s:%d\n", recv_data, bytes_received, inet_ntoa(address->sin_addr), htons(address->sin_port));
      break;
    }
  }
}
