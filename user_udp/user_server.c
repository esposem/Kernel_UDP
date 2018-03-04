#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include "user_udp.h"
#include "userver_operations.h"
#include "user_message.h"

int udps_socket;
static enum operations operation = PRINT;

static char * ipmy;
static int myport = 0;

int stop = 1;

void sig_handler(int signo) {
  if (signo == SIGINT){
    stop = 0;
  }
}

void check_args(int argc, char * argv[]){
  int narg = 3;
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
    }else{
      printf("%s not recognised", argv[i]);
    }
  }
  printf("\n");

  if(argc < narg){
    printf("Usage: %s ipaddress port -o [p | l | t | s]\n",argv[0]);
    exit(0);
  }

  ipmy = argv[1];
  myport = atoi(argv[2]);
}

void udp_init(void){

  struct sockaddr_in servaddr;

  if((udps_socket=socket(AF_INET,SOCK_DGRAM,0))<0) {
    perror("Error creating socket");
    exit(0);
  }

  struct timeval t;
  t.tv_sec = 1;
  t.tv_usec = 0;

  setsockopt(udps_socket, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family=AF_INET;
  servaddr.sin_port=htons(myport);
  servaddr.sin_addr.s_addr=inet_addr(ipmy);

  if(bind(udps_socket,(struct sockaddr *)&servaddr,sizeof(servaddr))<0){
    perror("Error binding socket.");
    exit(0);
  }

  printf("Server: Bind on %s:%d.\n", ipmy, myport);

}

void connection_handler(void){

  init_default_messages();
  message_data * rcv_buff = create_rcv_message();

  switch(operation){
    case LATENCY:
      latency(rcv_buff, reply, request);
      break;
    case TROUGHPUT:
      troughput(rcv_buff, request);
      break;
    case PRINT:
      print(rcv_buff, reply, request);
      break;
    default:
      server_simulation(rcv_buff, request);
      break;
  }

  delete_message(rcv_buff);
  del_default_messages();
}

int main(int argc,char *argv[]) {

  check_args(argc, argv);
  signal(SIGINT, sig_handler);
  udp_init();
  connection_handler();
  close(udps_socket);
  printf("Server closed\n");

  return 0;
}
