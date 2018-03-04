#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include "user_udp.h"
#include "user_message.h"
#include "uclient_operations.h"

int udpc_socket;

static char * serverip;
static int destport = 3000;

static char * ipmy;
static int myport = 0;

static enum operations operation = PRINT;
static unsigned long ns = 1;
static long tsec = -1;
static int nclients = 10;
int stop = 1;


void sig_handler(int signo) {
  if (signo == SIGINT){
    stop = 0;
  }
}

void check_args(int argc, char * argv[]){
  int narg = 5;
  printf("Parameters: ");
  for (int i = narg; i < argc; i++) {

    if(memcmp(argv[i], "-o",2) == 0 || memcmp(argv[i], "-O",2) == 0){
      i++;
      if(i < argc){
        printf("%s ", argv[i-1]);
        printf("%s ", argv[i]);
        if(argv[i][0] == 'p' || argv[i][0] == 'P'){
          operation = PRINT;
        }else if(argv[i][0] == 't' || argv[i][0] == 'T'){
          operation = TROUGHPUT;
        }else if(argv[i][0] == 'l' || argv[i][0] == 'L'){
          operation = LATENCY;
        }else if(argv[i][0] == 's' || argv[i][0] == 'S'){
          operation = SIMULATION;
        }
      }else{
        printf("\nError!\nUsage -o [t | l | p | s]\n");
        exit(0);
      }
      narg+=2;
    } else if(memcmp(argv[i], "-n",2) == 0 || memcmp(argv[i], "-N",2) == 0){
      i++;
      if(i < argc){
        printf("%s ", argv[i-1]);
        printf("%s ", argv[i]);
        ns = atol(argv[i]);
      }else{
        printf("\nError!\nUsage -n delay\n");
        exit(0);
      }
      narg+=2;
    }else if(memcmp(argv[i], "-t",2) == 0 || memcmp(argv[i], "-T",2) == 0){
      i++;
      if(i < argc){
        printf("%s ", argv[i-1]);
        printf("%s ", argv[i]);
        tsec = atol(argv[i]);
      }else{
        printf("\nError!\nUsage -t duration\n");
        exit(0);
      }
      narg+=2;
    }else if(memcmp(argv[i], "-c",2) == 0 || memcmp(argv[i], "-C",2) == 0){
      i++;
      if(i < argc){
        printf("%s ", argv[i-1]);
        printf("%s ", argv[i]);
        nclients = atol(argv[i]);
      }else{
        printf("\nError!\nUsage -c nclients\n");
        exit(0);
      }
      narg+=2;
    }else{
      printf("%s not recognised", argv[i]);
    }
  }
  printf("\n");

  if(argc < narg){
    printf("Usage: %s ipaddress port serveraddress serverport [-o (p | t | l | s) ] [-n microseconds] [-t duration] [-c nclients]\n",argv[0]);
    exit(0);
  }

  ipmy = argv[1];
  myport = atoi(argv[2]);
  serverip = argv[3];
  destport = atoi(argv[4]);
}

void udp_init(void){

  struct sockaddr_in cliaddr;

  if((udpc_socket=socket(AF_INET,SOCK_DGRAM,0))<0) {
    perror("Error creating socket");
    exit(0);
  }

  struct timeval t;
  t.tv_sec = 1;
  t.tv_usec = 0;

  setsockopt(udpc_socket, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));

  bzero(&cliaddr,sizeof(cliaddr));
  cliaddr.sin_family=AF_INET;
  cliaddr.sin_port=htons(myport);
  cliaddr.sin_addr.s_addr=inet_addr(ipmy);

  if(bind(udpc_socket,(struct sockaddr *)&cliaddr,sizeof(cliaddr))<0){
    perror("Error binding socket.");
    exit(0);
  }

  printf("Client: Bind on %s:%d.\n", ipmy, myport);

}

void connection_handler(void){

  struct sockaddr_in dest_addr;

  init_default_messages();
  message_data * rcv_buff = create_rcv_message();

  fill_sockaddr_in(&dest_addr, serverip, AF_INET, destport);

  switch(operation){
    case LATENCY:
      latency(rcv_buff, request, reply, &dest_addr);
      break;
    case TROUGHPUT:
      troughput(request, &dest_addr, ns, tsec);
      break;
    case PRINT:
      print(rcv_buff, request, reply, &dest_addr);
      break;
    default:
      client_simulation(rcv_buff, request, &dest_addr, nclients);
      break;
  }


  delete_message(rcv_buff);
  del_default_messages();

}

int main(int argc,char *argv[]) {
  check_args(argc, argv);
  signal(SIGINT, sig_handler);
  udp_init();
  printf("Client: Destination is %s:%d\n", serverip, destport);
  prepare_file(operation, nclients);
  connection_handler();
  close(udpc_socket);
  printf("Client closed\n");
  return 0;
}
