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
    if(memcmp(argv[i], "-p",2) == 0 || memcmp(argv[i], "-P",2) == 0){
      printf("%s ", argv[i]);
      narg++;
    }else if(memcmp(argv[i], "-t",2) == 0 || memcmp(argv[i], "-T",2) == 0){
      printf("%s ", argv[i]);
      operation = TROUGHPUT;
      narg++;
    }else if(memcmp(argv[i], "-l",2) == 0 || memcmp(argv[i], "-L",2) == 0){
      printf("%s ", argv[i]);
      operation = LATENCY;
      narg++;
    }
  }
  printf("\n");

  if(argc < narg){
    printf("Usage: %s ipaddress port [-p | -l | -t]\n",argv[0]);
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

  // if((connect(udps_socket, (struct sockaddr *)&serv,sizeof(serv))) < 0) {
  //   perror("ERROR connecting to server");
  //   exit(0);
  // }

}

void connection_handler(void){

  init_messages();
  message_data * rcv_buff = malloc(sizeof(message_data) + MAX_UDP_SIZE);
  message_data * send_buff = malloc(sizeof(message_data) + MAX_MESS_SIZE);

  rcv_buff->mess_len = MAX_UDP_SIZE;
  send_buff->mess_len = MAX_MESS_SIZE;
  memcpy(send_buff->mess_data, reply->mess_data, reply->mess_len);

  switch(operation){
    case LATENCY:
      latency(rcv_buff, send_buff, request);
      break;
    case TROUGHPUT:
      troughput(rcv_buff, request);
      break;
    default:
      print(rcv_buff, send_buff, request);
      break;
  }

  close(udps_socket);
  free(rcv_buff);
  free(send_buff);
  printf("Server closed\n");
}

int main(int argc,char *argv[]) {

  check_args(argc, argv);
  signal(SIGINT, sig_handler);
  udp_init();
  connection_handler();

  return 0;
}
