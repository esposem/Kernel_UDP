#include "kclient_operations.h"
#include "k_file.h"
#include "kernel_udp.h"
#include <linux/kthread.h>

unsigned long long sent = 0;

// time different in microseconds
static unsigned long
diff_time(struct timespec* op1, struct timespec* op2)
{
  return (op1->tv_sec - op2->tv_sec) * 1000000 +
         (op1->tv_nsec - op2->tv_nsec) / 1000;
}

#define DISCARD_INIT 100
#define DISCARD_END 100
#define SIZE_SAMPLE 10000
#define VERBOSE 1

struct client
{
  int                id;        // id the client
  int                busy;      // flag to check if it is able to send or not
  unsigned long      waiting;   // how much it's waiting for packet 0-1sec max
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

static void
init_stat(struct statistic* s)
{
  s->count = 0;
  s->data = kmalloc(sizeof(unsigned long) * SIZE_SAMPLE, GFP_KERNEL);
  s->elapsed = kmalloc(sizeof(unsigned long) * SIZE_SAMPLE, GFP_KERNEL);
  memset(s->data, 0, sizeof(unsigned long) * SIZE_SAMPLE);
  memset(s->elapsed, 0, sizeof(unsigned long) * SIZE_SAMPLE);
}

static void
del_stat(struct statistic* s)
{
  kfree(s->data);
  kfree(s->elapsed);
}

void
init_clients(struct client* cl, int n)
{
  memset(cl, 0, sizeof(struct client) * n);
  for (int i = 0; i < n; i++)
    cl[i].id = i;
}

static void
write_results(struct file* f, struct statistic* sample_lat,
              unsigned int nclients)
{
  char  data[256];
  char  filen[256];
  char* str = "#count\tlatency\tabsolute_time\n";

  printk(KERN_INFO "%s Writing results into a file...\n",
         get_service_name(cl_thread_1));
  if (!f) {
    printk(KERN_ERR "Cannot create file %s\n", filen);
    return;
  }

  sprintf(data, "# SMPL=%u, DSCD_I=%u, DSCD_E=%u, CL=%d\n", sample_lat->count,
          DISCARD_INIT, DISCARD_END, nclients);
  file_write(f, 0, data, strlen(data));
  file_write(f, 0, str, strlen(str));
  for (int i = DISCARD_INIT; i < sample_lat->count - DISCARD_END; i++) {
    sprintf(data, "%lu\t%lu\t%lu\n", 1L, sample_lat->data[i],
            sample_lat->elapsed[i]);
    file_write(f, 0, data, strlen(data));
  }

  file_write(f, 0, "\n#END#\n", 7);
  file_close(f);
  printk("%s Done\n", get_service_name(cl_thread_1));
}

void
run_outstanding_clients(message_data* rcv_buf, message_data* send_buf,
                        struct sockaddr_in* dest_addr, unsigned int nclients,
                        unsigned int duration, struct file* f)
{
  char *recv_data = get_message_data(rcv_buf),
       *send_data = get_message_data(send_buf);

  size_t recv_size = get_total_mess_size(rcv_buf),
         send_size = get_message_size(send_buf),
         size_mess = get_total_mess_size(send_buf);

  if (recv_size < size_mess) {
    printk(KERN_ERR
           "%s Error, receiving buffer size is smaller than expected message\n",
           get_service_name(cl_thread_1));
    return;
  }

  struct socket*   sock = get_service_sock(cl_thread_1);
  int              bytes_received, bytes_sent, id;
  unsigned long    total_received = 0, elapsed = 0;
  struct msghdr    hdr, reply;
  struct client*   cl = kmalloc(sizeof(struct client) * nclients, GFP_KERNEL);
  struct timespec  init_second, current_time;
  struct statistic sample_lat;

  printk("%s Started simulation with %u clients\n",
         get_service_name(cl_thread_1), nclients);

  init_stat(&sample_lat);
  init_clients(cl, nclients);
  construct_header(&hdr, dest_addr);
  construct_header(&reply, NULL);
  getrawmonotonic(&init_second);

  do {
    for (int i = 0; i < nclients; i++) {
      if (cl[i].busy)
        continue;
      set_message_id(send_buf, i);

      getrawmonotonic(&cl[i].start_lat);
      if ((bytes_sent = udp_send(sock, &hdr, send_buf, size_mess)) !=
          size_mess) {
        printk(KERN_ERR "%s Error %d in sending: server not active\n",
               get_service_name(cl_thread_1), bytes_sent);
        continue;
      }
      cl[i].busy = 1;
    }

    bytes_received = udp_receive(sock, &reply, rcv_buf, recv_size);
    getrawmonotonic(&current_time);
    elapsed = diff_time(&current_time, &init_second);

    // check message has been received correctly
    if (bytes_received == size_mess &&
        memcmp(recv_data, send_data, send_size) == 0 &&
        (id = get_message_id(rcv_buf)) < nclients) {

      // update client data
      cl[id].total_received++;
      total_received++;
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

    if (VERBOSE && total_received % (10000 * nclients) == 0)
      printk(KERN_INFO "%s %ld messages in %ld msec.\n\n",
             get_service_name(cl_thread_1), total_received, elapsed / 1000);
  } while (!(kthread_should_stop() || signal_pending(current)) &&
           (elapsed / 1000000) < duration);

  write_results(f, &sample_lat, nclients);

  del_stat(&sample_lat);
  kfree(cl);
}
