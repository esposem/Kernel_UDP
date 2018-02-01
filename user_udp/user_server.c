#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <event2/event.h>
#include "include/user_udp.h"

char * in_buf, * out_buf;
int sockfd,len,receivedbytes;
struct sockaddr_in servaddr,cliaddr;
struct sockaddr_storage client_address;

#if TEST == 1
  unsigned long long received = 0, rec_min = 0, seconds = 0,res;
  struct timeval departure_time,arrival_time;
  double average = 0;
#endif

void sig_handler(int signo) {
  if (signo == SIGINT){
    close(sockfd);
    free(in_buf);
    free(out_buf);
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
  struct timeval t;
  t.tv_sec = 0;
  t.tv_usec = MAX_RCV_WAIT;

  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
             &t, sizeof(t));

  printf("Server: UDP Socket Created Successfully.\n");

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
  in_buf = malloc(MAX_UDP_SIZE);
  out_buf = malloc(MAX_MESS_SIZE);
  memset(out_buf, '\0', MAX_MESS_SIZE);
  memcpy(out_buf, OK, strlen(OK) + 1);

  #if TEST == 1
    gettimeofday(&departure_time,NULL);
  #endif

  while(1) {

    memset(in_buf, 0, MAX_UDP_SIZE);
    receivedbytes=recvfrom(sockfd,in_buf,MAX_UDP_SIZE,0, (struct sockaddr *) &client_address,&len);
    if (receivedbytes == MAX_MESS_SIZE && memcmp(HELLO, in_buf, MAX_MESS_SIZE) == 0){
      #if TEST != 1
        #if TEST == 0
          memset(host, 0,NI_MAXHOST);
          memset(service,0, NI_MAXSERV);

          s = getnameinfo((struct sockaddr *) &client_address,
                               len, host, NI_MAXHOST,
                               service, NI_MAXSERV, NI_NUMERICSERV);
          printf("Server: Received %s (%d bytes) from %s:%s\n", in_buf, receivedbytes, host, service);
        #endif

        if((sendto(sockfd,out_buf,receivedbytes,0,(struct sockaddr *)&client_address,len))<0){
            perror("CANNOT SENT BACK");
        }else{
          #if TEST == 0
            printf("Server: Sent OK\n");
          #endif
          // break;
        }
      #else
        rec_min++;
        gettimeofday(&arrival_time, NULL);
        res = (arrival_time.tv_sec * 1000000 + arrival_time.tv_usec) - (departure_time.tv_sec * 1000000 + departure_time.tv_usec );
        if(res >= 1000000){
          seconds ++;
          received +=rec_min;
          average = (double)received/(double)seconds;
          printf("Server: Total packages received in a second: %llu \t Average %.2f/sec\n",rec_min,average );
          rec_min = 0;
          gettimeofday(&departure_time, NULL);
        }
      #endif
    }

  }

  return 0;
}
