#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include "user_udp.h"
#include "uclient_operations.h"

int udpc_socket;
static enum operations operation = PRINT;

static char * serverip;
static int destport = 3000;

static char * ipmy;
static int myport = 0;

static unsigned long ns = 1;
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
    }else if(memcmp(argv[i], "-n",2) == 0 || memcmp(argv[i], "-N",2) == 0){
      printf("%s ", argv[i]);
      i++;
      if(i < argc){
        printf("%s ", argv[i]);
        ns = atoi(argv[i]);
      }else{
        printf("\nError!\nUsage: %s ipaddress port serveraddress serverport [-p | -l | -t] [-u microseconds]\n",argv[0]);
        exit(0);
      }
      narg+=2;
    }
  }
  printf("\n");

  if(argc < narg){
    printf("Usage: %s ipaddress port serveraddress serverport [-p | -l | -t] [-u microseconds]\n",argv[0]);
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

  // if((connect(udpc_socket, (struct sockaddr *)&serv,sizeof(serv))) < 0) {
  //   perror("ERROR connecting to server");
  //   exit(0);
  // }

}

void connection_handler(void){

  struct sockaddr_in dest_addr;

  init_messages();
  message_data * rcv_buff = malloc(sizeof(message_data) + MAX_UDP_SIZE);
  message_data * send_buff = malloc(sizeof(message_data) + MAX_MESS_SIZE);

  rcv_buff->mess_len = MAX_UDP_SIZE;
  send_buff->mess_len = MAX_MESS_SIZE;
  memcpy(send_buff->mess_data, request->mess_data, request->mess_len);

  dest_addr.sin_family=AF_INET;
  dest_addr.sin_port=htons(destport);
  dest_addr.sin_addr.s_addr=inet_addr(serverip);

  switch(operation){
    case LATENCY:
      latency(rcv_buff, send_buff, reply, &dest_addr);
      break;
    case TROUGHPUT:
      troughput(send_buff, &dest_addr, ns);
      break;
    default:
      print(rcv_buff, send_buff, reply, &dest_addr);
      break;
  }

  close(udpc_socket);
  free(rcv_buff);
  free(send_buff);
  printf("Client closed\n");
}

int main(int argc,char *argv[]) {

  check_args(argc, argv);
  signal(SIGINT, sig_handler);
  udp_init();
  connection_handler();

  return 0;
}
