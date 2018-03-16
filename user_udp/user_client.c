#include "uclient_operations.h"
#include "user_message.h"
#include "user_udp.h"
#include <arpa/inet.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

int udpc_socket = -1;

// input parameters and default values
static const char* myip = "127.0.0.1";
static const char* serverip = "127.0.0.1";
static int         destport = 3000;
static int         myport = 4000;
static int         msg_size = 128; // not used
static int         nclients = 1;
static int         verbose = 0;
static long        tsec = 30;

static unsigned long ns = 1;
static int           ntests = 9;
int                  stop = 1;

void
sig_handler(int signo)
{
  if (signo == SIGINT)
    stop = 0;
}

void
check_args(int argc, char* argv[])
{
  int                  opt = 0, idx = 0;
  static struct option options[] = {
    { "my-port", required_argument, 0, 'm' },
    { "my-ip", required_argument, 0, 'a' },
    { "server-ip", required_argument, 0, 'i' },
    { "server-port", required_argument, 0, 'p' },
    { "message-size", required_argument, 0, 's' },
    { "clients", required_argument, 0, 'c' },
    { "duration", required_argument, 0, 'd' },
    { "verbose", no_argument, 0, 'v' },
    { "help", no_argument, 0, 'h' },
    { 0, 0, 0, 0 }
  };

  while ((opt = getopt_long(argc, argv, "hvm:i:s:c:a:p:d:", options, &idx)) !=
         -1) {
    switch (opt) {
      case 'a':
        myip = optarg;
        break;
      case 'i':
        serverip = optarg;
        break;
      case 'c':
        nclients = atoi(optarg);
        break;
      case 'p':
        destport = atoi(optarg);
        break;
      case 's':
        msg_size = atoi(optarg);
        break;
      case 'd':
        tsec = atol(optarg);
        break;
      case 'm':
        myport = atoi(optarg);
        break;
      case 'v':
        verbose = 1;
        break;
      default:
        printf("Usage: %s -a <my-ip> -m <my-port> -i <server-ip> -p "
               "<server-port> -c <nr-outstanding-clients> -d <duration>\n",
               argv[0]);
        exit(1);
    }
  }

  printf("Simulating %d clients listening on %s:%d,\nconnecting to %s:%d for "
         "%ld seconds.\n\n",
         nclients, myip, myport, serverip, destport, tsec);
}

void
udp_init(void)
{
  struct sockaddr_in cliaddr;

  if ((udpc_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Error creating socket");
    exit(0);
  }

  struct timeval t;
  t.tv_sec = 0;
  t.tv_usec = 100000;

  setsockopt(udpc_socket, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
  bzero(&cliaddr, sizeof(cliaddr));
  cliaddr.sin_family = AF_INET;
  cliaddr.sin_port = htons(myport);
  cliaddr.sin_addr.s_addr = inet_addr(myip);

  if (bind(udpc_socket, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) < 0) {
    perror("Error binding socket.");
    exit(1);
  }

  printf("Client: Bind on port %d.\n", myport);
}

void
connection_handler(void)
{
  struct sockaddr_in dest_addr;
  message_data*      rcv_buff;

  init_default_messages();
  rcv_buff = create_rcv_message();
  fill_sockaddr_in(&dest_addr, serverip, AF_INET, destport);
  run_outstanding_clients(rcv_buff, request, &dest_addr, nclients, tsec,
                          verbose);
  delete_message(rcv_buff);
  del_default_messages();
}

int
main(int argc, char* argv[])
{
  check_args(argc, argv);
  signal(SIGINT, sig_handler);
  udp_init();
  connection_handler();
  close(udpc_socket);
  printf("Client closed\n");
  return 0;
}
