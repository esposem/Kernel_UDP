#include "user_message.h"
#include "user_udp.h"
#include "userver_operations.h"
#include <arpa/inet.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

int udps_socket;
int stop = 1;

static int myport = 3000; // default port
static int verbose = 0;

void sig_handler(int signo)
{
  if (signo == SIGINT)
    stop = 0;
}

void check_args(int argc, char *argv[])
{
  int opt = 0, idx = 0;
  static struct option options[] = {
      {"port", required_argument, 0, 'p'},
      {"verbose", no_argument, 0, 'v'},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "hv:m:i:s:c:p:", options, &idx)) != -1)
  {
    switch (opt)
    {
    case 'p':
      myport = atoi(optarg);
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      printf("Usage: %s -p <port> --verbose\n", argv[0]);
      exit(1);
    }
  }
  printf("Server starting on port %d.\n", myport);
}

void udp_init(void)
{
  struct sockaddr_in servaddr;

  if ((udps_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("Error creating socket");
    exit(0);
  }

  struct timeval t;
  t.tv_sec = 1;
  t.tv_usec = 0;

  setsockopt(udps_socket, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(myport);
  servaddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(udps_socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
  {
    perror("Error binding socket.");
    exit(0);
  }

  printf("Server: Bind on port %d.\n", myport);
}

void connection_handler(void)
{

  init_default_messages();
  message_data *rcv_buff = create_rcv_message();
  echo_server(rcv_buff, request);
  delete_message(rcv_buff);
  del_default_messages();
}

int main(int argc, char *argv[])
{

  check_args(argc, argv);
  signal(SIGINT, sig_handler);
  udp_init();
  connection_handler();
  close(udps_socket);
  printf("Server closed.\n");
  return 0;
}