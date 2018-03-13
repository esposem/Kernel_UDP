#include "uclient_operations.h"
#include "user_udp.h"
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define SIZE_SAMPLE 10000000
#define DISCARD_INIT 100
#define DISCARD_END 100

struct client
{
  int                id;        // id the client
  int                busy;      // flag to check if it is able to send or not
  long               waiting;   // how much it's waiting for packet 0-1sec max
  struct timespec    start_lat; // keeps track of sample latency
  unsigned long long total_received;
};

struct statistic
{
  unsigned long* data;
  unsigned long* elapsed;
  int            count;
};

static int TIMEOUT_US = 500; // max amount a message can wait in us

// time diff in microseconds
static unsigned long
diff_time(struct timespec* op1, struct timespec* op2)
{
  return (op1->tv_sec - op2->tv_sec) * 1e6 +
         (op1->tv_nsec - op2->tv_nsec) * 1e-3;
}

static void
init_stat(struct statistic* s, size_t size)
{
  s->count = 0;
  s->data = malloc(sizeof(unsigned long) * size);
  s->elapsed = malloc(sizeof(unsigned long) * size);
  memset(s->data, 0, sizeof(unsigned long) * size);
  memset(s->elapsed, 0, sizeof(unsigned long) * size);
}

static void
del_stat(struct statistic* s)
{
  free(s->elapsed);
  free(s->data);
}

static void
write_results(int nclients, struct client* cl, struct statistic* troughput,
              struct statistic* sample_lat)
{
  printf("Client: Writing results into a file...\n");
  char filen[100];
  sprintf(filen, "./results/user_data/results-process-%d.txt", nclients);

  int f = open(filen, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
  if (f < 0) {
    printf("File error\n");
    return;
  }

  char  data[256];
  char* str = "#count\tlatency\tabsolute_time\n";

  sprintf(data, "# SMPL=%u, DSCD_I=%u, DSCD_E=%u, CL=%d\n", sample_lat->count,
          DISCARD_INIT, DISCARD_END, nclients);
  write(f, data, strlen(data));
  write(f, str, strlen(str));
  for (int i = DISCARD_INIT; i < sample_lat->count - DISCARD_END; i++) {
    sprintf(data, "%lu\t%lu\t%lu\n", 1L, sample_lat->data[i],
            sample_lat->elapsed[i]);
    write(f, data, strlen(data));
  }

  write(f, "\n#END#\n", 7);
  close(f);
  printf("Client: Done\n");
}

void
init_clients(struct client* cl, int n)
{
  memset(cl, 0, n * sizeof(struct client));
  for (int i = 0; i < n; i++)
    cl[i].id = i;
}

void
run_outstanding_clients(message_data* rcv_buf, message_data* send_buf,
                        struct sockaddr_in* dest_addr, unsigned int nclients,
                        long duration, int verbose)
{
  char *recv_data = get_message_data(rcv_buf),
       *send_data = get_message_data(send_buf);

  size_t recv_size = get_total_mess_size(rcv_buf),
         send_size = get_message_size(send_buf),
         size_mess = get_total_mess_size(send_buf);

  if (recv_size < size_mess) {
    printf("Client: Error, receiving buffer size is smaller than expected "
           "message\n");
    return;
  }

  int              bytes_received, bytes_sent, id;
  unsigned long    diff = 0;
  struct msghdr    hdr, reply;
  struct client*   cl = malloc(sizeof(struct client) * nclients);
  struct timespec  init_second, current_time;
  struct statistic sample_lat;
  int              lat_pick = 0;
  long             total_received = 0, elapsed = 0;
  struct iovec     iovreply[1], iov[1];

  printf("Client: Starting with %u clients\n", nclients);
  init_stat(&sample_lat, SIZE_SAMPLE);
  init_clients(cl, nclients);
  clock_gettime(CLOCK_MONOTONIC_RAW, &init_second);
  construct_header(&hdr, dest_addr);
  construct_header(&reply, NULL);
  fill_hdr(&hdr, iov, send_buf, size_mess);
  fill_hdr(&reply, iovreply, rcv_buf, recv_size);

  do {
    for (int i = 0; i < nclients; i++) {
      if (cl[i].busy)
        continue;

      clock_gettime(CLOCK_MONOTONIC_RAW, &cl[i].start_lat);
      set_message_id(send_buf, i);
      if (sendmsg(udpc_socket, &hdr, 0) != size_mess) {
        perror("ERROR IN SENDTO");
        continue;
      }

      cl[i].busy = 1; // just sent a message
    }

    bytes_received = recvmsg(udpc_socket, &reply, MSG_WAITALL);
    total_received++;
    clock_gettime(CLOCK_MONOTONIC_RAW, &current_time);
    elapsed = diff_time(&current_time, &init_second);

    // check message has been received correctly
    if (bytes_received == size_mess &&
        memcmp(recv_data, send_data, send_size) == 0 &&
        (id = get_message_id(rcv_buf)) < nclients) {

      // update client data
      cl[id].total_received++;
      sample_lat.count = sample_lat.count + 1 == SIZE_SAMPLE
                           ? sample_lat.count
                           : sample_lat.count + 1;
      sample_lat.elapsed[sample_lat.count] = elapsed;
      sample_lat.data[sample_lat.count] =
        diff_time(&current_time, &cl[id].start_lat);
      cl[id].busy = 0;
    }

    for (int i = 0; i < nclients; i++)
      if (cl[i].busy &&
          diff_time(&current_time, &cl[i].start_lat) > TIMEOUT_US) {
        cl[i].waiting++;
        cl[i].busy = 0;
        if (cl[i].waiting > 1) { // more than two lost messages in a row
          TIMEOUT_US += 100;
        }
      }

    if (verbose && total_received % (5000 * nclients) == 0)
      printf("%ld messages in %ld msec.\n\n", total_received, elapsed / 1000);
  } while (stop && (elapsed * 1e-6 < duration));

  write_results(nclients, cl, NULL, &sample_lat);
  del_stat(&sample_lat);
  free(cl);
}
