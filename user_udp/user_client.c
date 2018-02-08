#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include "include/user_udp.h"

static char * in_buf, * out_buf;
static int sockfd,len, bytes_received, bytes_sent,message_received = 1;
static struct sockaddr_in serv,cliaddr;
static enum operations operation = PRINT;
static unsigned long frac_sec = 1;
unsigned long long sent = 0,total = 0,counted = 0;

void sig_handler(int signo) {
  if (signo == SIGINT){
    close(sockfd);
    free(in_buf);
    free(out_buf);
    if(operation == THROUGHPUT){ // #if TEST == 1
      printf("Client: Total number of sent packets: %llu\n", sent);
    } // #endif
    printf("Client closed\n");
  }
  exit(0);
}


int main(int argc,char *argv[]) {

  int narg = 5;
  for (int i = 0; i < argc; i++) {
    if(memcmp(argv[i], "-p",2) == 0 || memcmp(argv[i], "-P",2)){
      narg++;
    }else if(memcmp(argv[i], "-t",2) == 0 || memcmp(argv[i], "-T",2) == 0){
      operation = THROUGHPUT;
      narg++;
    }else if(memcmp(argv[i], "-l",2) == 0 || memcmp(argv[i], "-L",2) == 0){
      operation = LATENCY;
      narg++;
    }
    // else if(memcmp(argv[i], ))

  }

  if(argc != narg){
    printf("Usage: %s ipaddress port serveraddress serverport [-p | -l | -t]\n",argv[0]);
    exit(0);
  }

  signal(SIGINT, sig_handler);

  if((sockfd=socket(AF_INET,SOCK_DGRAM,0))<0) {
    perror("Error creating socket");
    exit(0);
  }

  // Make the recvfrom block for only 100 ms
  struct timeval t;
  t.tv_sec = 0;
  t.tv_usec = 100000;

  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
             &t, sizeof(t));

  bzero(&cliaddr,sizeof(cliaddr));
  cliaddr.sin_family=AF_INET;
  cliaddr.sin_port=htons(atoi(argv[2]));
  cliaddr.sin_addr.s_addr=inet_addr(argv[1]);

  if(bind(sockfd,(struct sockaddr *)&cliaddr,sizeof(cliaddr))<0){
    perror("Error binding socket.");
    exit(0);
  }

  printf("Client: Bind on %s:%s.\n", argv[1], argv[2]);

  memset(&serv,0,sizeof(serv));
  serv.sin_family=AF_INET;
  serv.sin_port=htons(atoi(argv[4]));
  serv.sin_addr.s_addr=inet_addr(argv[3]);

  // if((connect(sockfd, (struct sockaddr *)&serv,sizeof(serv))) < 0) {
  //   perror("ERROR connecting to server");
  //   exit(0);
  // }

  printf("Client: Connected Successfully.\n");

  in_buf = malloc(MAX_UDP_SIZE);
  out_buf = malloc(MAX_MESS_SIZE);
  memcpy(out_buf, HELLO, strlen(HELLO)+1);

  if(operation == PRINT){ // #if TEST == 0
    if((sendto(sockfd,out_buf,strlen(HELLO)+1,0,(struct sockaddr *)&serv,sizeof(serv)))<0) {
      perror("ERROR IN SENDTO");
    }
    printf("Client: sent HELLO\n");
  } // #endif

  struct timeval departure_time,arrival_time, seconds_time;
  double average = 0;
  unsigned long long res;
  gettimeofday(&departure_time,NULL);
  gettimeofday(&seconds_time,NULL);
  unsigned long logn sent_sec = 0, seconds = 0, intervals = 0;

  while(1){

    if(operation != PRINT){ // #if TEST != 0
      if(operation == LATENCY){ // #if TEST == 2
        if(message_received){
          if((sendto(sockfd,out_buf,strlen(HELLO)+1,0,(struct sockaddr *)&serv,sizeof(serv)))<0) {
            perror("ERROR IN SENDTO");
            exit(0);
          }
          message_received = 0;
        }
      }else{ // #else
        gettimeofday(&arrival_time, NULL);
        res = ((arrival_time.tv_sec * _1_SEC) + arrival_time.tv_usec) - ((departure_time.tv_sec * _1_SEC) + departure_time.tv_usec );
        if(res >= _1_SEC/frac_sec){
          gettimeofday(&departure_time, NULL);
          bytes_sent = sendto(sockfd,out_buf,strlen(HELLO)+1,0,(struct sockaddr *)&serv,sizeof(serv));
          if(bytes_sent == MAX_MESS_SIZE){
            sent_sec++;
          }else{
            perror("ERROR IN SENDTO");
          }
          intervals++;
          if(intervals == frac_sec){
            seconds ++;
            sent +=sent_sec;
            average = (double)sent/(double)seconds;
            printf("Client: Sent %lld/sec \t Average %.3f/sec\n",sent_sec,average );
            sent_sec = 0;
            intervals = 0;
          }
        // bytes_sent = sendto(sockfd,out_buf,strlen(HELLO)+1,0,(struct sockaddr *)&serv,sizeof(serv));
        // if(bytes_sent < 0) {
        //   perror("ERROR IN SENDTO");
        // }else if(bytes_sent == MAX_MESS_SIZE){
        //   sent_sec++;
        //   gettimeofday(&arrival_time, NULL);
        //   res = ((arrival_time.tv_sec * _1_SEC) + arrival_time.tv_usec) - ((departure_time.tv_sec * _1_SEC) + departure_time.tv_usec );
        //   if(res >= _1_SEC){
        //     seconds ++;
        //     sent +=sent_sec;
        //     average = (double)sent/(double)seconds;
        //     printf("Client: Sent %lld/sec \t Average %.3f/sec\n",sent_sec,average );
        //     sent_sec = 0;
        //     gettimeofday(&departure_time, NULL);
        //   }
        // }
      } // #endif
    } // #endif

    if(operation != THROUGHPUT){ // #if TEST != 1
      memset(in_buf, 0, MAX_UDP_SIZE);
      bytes_received = recvfrom(sockfd,in_buf,MAX_UDP_SIZE,0,(struct sockaddr *)&cliaddr,&len);

      if(bytes_received == MAX_MESS_SIZE && memcmp(in_buf, OK, strlen(OK)+1) == 0){
        if(operation == LATENCY){ // #if TEST == 2
          gettimeofday(&arrival_time,NULL);
          res = ((arrival_time.tv_sec * _1_SEC) + arrival_time.tv_usec) - ((departure_time.tv_sec * _1_SEC) + departure_time.tv_usec );
          total += res;
          counted ++;
          average = (double)total/ (double)counted;
          res = ((arrival_time.tv_sec * _1_SEC) + arrival_time.tv_usec) - ((seconds_time.tv_sec * _1_SEC) + seconds_time.tv_usec );
          if(res >= _1_SEC){
            printf("Client Latency average is %.3f\n", average );
            gettimeofday(&seconds_time, NULL);
          }
          gettimeofday(&departure_time,NULL);
          message_received = 1;
        }else{ // #else
          printf("Client: Received %s (%d bytes) from %s:%s\n", in_buf, bytes_received, argv[1], argv[2]);
          break;
        } // #endif
      }
    } // #endif
  }

  return 0;

}
