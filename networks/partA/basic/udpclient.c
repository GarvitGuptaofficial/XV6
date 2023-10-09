#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv){

  if (argc != 2) {
    printf("Usage: %s <port>\n", argv[0]);
    exit(0);
  }

  char *ip = "127.0.0.1";
  int port = atoi(argv[1]);

  int sockfd;
  struct sockaddr_in addr;
  char buffer[1024];
  socklen_t addr_size;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfd<0){
     perror("[-]Socket error");
    exit(1);
  }

  memset(&addr, '\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip);
   
  bzero(buffer, 1024);
  strcpy(buffer, "Hello, World!");
  int y=sendto(sockfd, buffer, 1024, 0, (struct sockaddr*)&addr, sizeof(addr));
  if(y<0){
     printf("sendto failed\n");
     return -1;
  }
  printf("[+]Data send: %s\n", buffer);

  bzero(buffer, 1024);
  addr_size = sizeof(addr);
  int m=recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*)&addr, &addr_size);
  if(m<0){
     printf("recvfrom failed\n");
     return -1;
  }
  printf("[+]Data recv: %s\n", buffer);
  close(sockfd);
  printf("Client CLosed\n");
  return 0;
}