#include<stdio.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include "include/user_udp.h"


int main(int argc,char *argv[]) {

  int sockfd,len, bytes_received;
  struct sockaddr_in serv,cliaddr;
  char buff[SIZE_BUFF];

  if(argc != 3){
    printf("Usage: %s ipaddress port\n",argv[0]);
    exit(0);
  }

  if((sockfd=socket(AF_INET,SOCK_DGRAM,0))<0) {
    perror("error creating socket");
    exit(0);
  }

  // Make the recvfrom block for only 100 ms
  // struct timeval t;
  // t.tv_sec = 0;
  // t.tv_usec = 100000;
  //
  // setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
  //            &t, sizeof(t));

  memset(&serv,0,sizeof(serv));
  serv.sin_family=AF_INET;
  serv.sin_port=htons(atoi(argv[2]));
  serv.sin_addr.s_addr=inet_addr(argv[1]);

  if((connect(sockfd, (struct sockaddr *)&serv,sizeof(serv))) < 0) {
    perror("ERROR connecting to server");
    exit(0);
  }

  printf("Client: Connected Successfully.\n");


  #if TEST == 0
    // 0: send HELLO
    memcpy(buff, HELLO, SIZE_BUFF);
    if((sendto(sockfd,buff,SIZE_BUFF,0,(struct sockaddr *)&serv,sizeof(serv)))<0) {
      perror("ERROR IN SENDTO");
    }
    printf("Client: sent HELLO\n");
  #endif

  #if TEST == 1
    long long sent = 0;
    long long sent_min = 0;
    //TODO Implement timer
  #endif

  #if TEST == 2
    double average = 0;
    long long total = 0;
    long long counted = 0;
    struct timeval departure_time,arrival_time;
    gettimeofday(&departure_time,NULL);
  #endif

  while(1){
    #if TEST != 0
      // 1: send forever
      // 2: send HELLO
      memcpy(buff, HELLO, SIZE_BUFF);
      if((sendto(sockfd,buff,SIZE_BUFF,0,(struct sockaddr *)&serv,sizeof(serv)))<0) {
        perror("ERROR IN SENDTO");
      }
      #if TEST == 1
        sent++;
      #endif
    #endif

    #if TEST != 1
      // 0: receive OK, exit
      // 2: receive OK, resend
      memset(buff, 0, SIZE_BUFF);
      bytes_received = recvfrom(sockfd,buff,SIZE_BUFF,0,(struct sockaddr *)&cliaddr,&len);

      if(bytes_received == SIZE_BUFF && memcmp(buff, OK, strlen(OK) + 1) == 0){
        #if TEST == 2
          // 2: calculate LATENCY
          gettimeofday(&arrival_time,NULL);
          long res = (arrival_time.tv_sec * 1000000 + arrival_time.tv_usec) - (departure_time.tv_sec * 1000000 + departure_time.tv_usec );
          total += res;
          counted ++;
          average = (double)total/ (double)counted;
          printf("\rLATENCY: %ld microseconds \t Average %f",res, average );
          gettimeofday(&departure_time,NULL);
        #else
          // 0: exit
          printf("Client: Received %s from server\n",buff);
          break;
        #endif
      }
    #endif
  }

  close(sockfd);
  #if TEST == 1
    printf("Total number of sent packets: %lu\n", sent);
  #endif
  printf("Client closed\n");
  return 0;

}
