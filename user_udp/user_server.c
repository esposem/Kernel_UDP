#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<string.h>
#include<unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "include/user_udp.h"

int main(int argc,char *argv[]) {

  char buf[SIZE_BUFF];
  int sockfd,len,receivedbytes;
  struct sockaddr_in servaddr,cliaddr;
  struct sockaddr_storage client_address;

  if(argc != 2){
    printf("Usage: %s port\n",argv[0]);
    exit(0);
  }

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
    long long received = 0;
    long long rec_min = 0;
    //TODO Implement timer
  #endif

  while(1) {
    memset(buf, 0, SIZE_BUFF);
    if((receivedbytes=recvfrom(sockfd,buf,SIZE_BUFF,0, (struct sockaddr *) &client_address,&len))<0){
      perror("ERROR");
    }

    memset(host, 0,NI_MAXHOST);
    memset(service,0, NI_MAXSERV);

    s = getnameinfo((struct sockaddr *) &client_address,
                         len, host, NI_MAXHOST,
                         service, NI_MAXSERV, NI_NUMERICSERV);

    if (receivedbytes == SIZE_BUFF && memcmp(HELLO, buf, SIZE_BUFF) == 0){
      #if TEST != 1
        #if TEST == 0
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
      #else
        received++;
      #endif
    }else{
      perror("Server: message incomplete\n");
    }
  }

  close(sockfd);
  #if TEST == 1
    printf("Total number of received packets: %lu\n", received);
  #endif
  printf("Server closed\n");

  return 0;
}
