#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <event2/event.h>
#include "include/user_udp.h"

char buf[SIZE_BUFF];
int sockfd,len,receivedbytes;
struct sockaddr_in servaddr,cliaddr;
struct sockaddr_storage client_address;

#if TEST == 1
  long long received = 0, rec_min = 0, seconds = 0;
  struct timeval departure_time,arrival_time;
  unsigned long long res;
  double average = 0;
#endif

void sig_handler(int signo) {
  if (signo == SIGINT){
    close(sockfd);
    #if TEST == 1
      printf("Total number of received packets: %llu\n", received);
    #endif
    printf("Server closed\n");
  }
  exit(0);
}

int main(int argc,char *argv[]) {

  if(argc != 2){
    printf("Usage: %s port\n",argv[0]);
    exit(0);
  }

  signal(SIGINT, sig_handler);

  if((sockfd=socket(AF_INET,SOCK_DGRAM,0))<0)
  {
      printf("Error creating socket\n");
      exit(0);
  }

  // Make the recvfrom block for only 100 ms
  // struct timeval t;
  // t.tv_sec = 0;
  // t.tv_usec = 100000;
  //
  // setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
  //            &t, sizeof(t));

  printf("UDP Server Socket Created Successfully.\n");

  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family=AF_INET;
  servaddr.sin_port=htons(atoi(argv[1]));
  servaddr.sin_addr.s_addr=htonl(INADDR_ANY);

  if(bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))<0)
  {
      perror("Error binding socket.");
      exit(0);
  }

  char host[NI_MAXHOST], service[NI_MAXSERV];

  int s = getnameinfo((struct sockaddr *) &servaddr,
                       sizeof(servaddr), host, NI_MAXHOST,
                       service, NI_MAXSERV, NI_NUMERICSERV);

  if (s == 0){
    printf("Server: Bind on %s in any port.\n", host);
  }else{
    perror("Server: Could not get my address\n");
  }

  len=sizeof(struct sockaddr_storage);
  #if TEST == 1
    gettimeofday(&departure_time,NULL);
  #endif

  while(1) {
    memset(buf, 0, SIZE_BUFF);
    if((receivedbytes=recvfrom(sockfd,buf,SIZE_BUFF,0, (struct sockaddr *) &client_address,&len))<0){
      perror("ERROR");
    }

    #if TEST != 1
      if (receivedbytes == SIZE_BUFF && memcmp(HELLO, buf, SIZE_BUFF) == 0){

        #if TEST == 0
          memset(host, 0,NI_MAXHOST);
          memset(service,0, NI_MAXSERV);

          s = getnameinfo((struct sockaddr *) &client_address,
                               len, host, NI_MAXHOST,
                               service, NI_MAXSERV, NI_NUMERICSERV);
          printf("Received %s (%ld bytes) from %s:%s\n", buf, (long) receivedbytes, host, service);
        #endif
        memset(buf, 0, SIZE_BUFF);
        memcpy(buf, OK, strlen(OK) + 1);

        if((sendto(sockfd,buf,receivedbytes,0,(struct sockaddr *)&client_address,len))<0){
            perror("NOTHING SENT");
        }else{
          #if TEST == 0
            printf("Sent OK\n");
          #endif
          // break;
        }
      }else{
        perror("Server: message incomplete\n");
      }
    #else
      else{
        rec_min++;
        gettimeofday(&arrival_time, NULL);
        res = (arrival_time.tv_sec * 1000000 + arrival_time.tv_usec) - (departure_time.tv_sec * 1000000 + departure_time.tv_usec );
        if(res >= 1000000){
          seconds ++;
          received +=rec_min;
          average = (double)received/(double)seconds;
          printf("Total packages received in a second: %lld \t Average %f/sec\n",rec_min,average );
          rec_min = 0;
          gettimeofday(&departure_time, NULL);
        }
      }
    #endif

  }

  return 0;
}
