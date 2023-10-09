#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//1 for rock
//2 for scissor
//3 for paper
//4 for exit


int main(int argc, char **argv){

  if (argc != 3){
    printf("Usage: %s <port>\n", argv[0]);
    exit(0);
  }

  char *ip = "127.0.0.1";
  int port = atoi(argv[1]);

  int sockfd;
  struct sockaddr_in server_addr, client_addr;
  char buffer1[500];
  char buffer2[500];
  socklen_t addr_size;
  int n;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0){
    perror("[-]socket error");
    exit(1);
  }

  memset(&server_addr, '\0', sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip);

  n = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if (n < 0) {
    perror("[-]bind error");
    exit(1);
  }
  
  int sockfd1;
  struct sockaddr_in server_addr1, client_addr1;
  sockfd1 = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd1 < 0){
    perror("[-]socket error");
    exit(1);
  }
  
  int port2=atoi(argv[2]);
  memset(&server_addr1, '\0', sizeof(server_addr1));
  server_addr1.sin_family = AF_INET;
  server_addr1.sin_port = htons(port2);
  server_addr1.sin_addr.s_addr = inet_addr(ip);

  n = bind(sockfd1, (struct sockaddr*)&server_addr1, sizeof(server_addr1));
  if (n < 0) {
    perror("[-]bind error");
    exit(1);
  }



while(1){
    int r1,r2;
    int*w1;
    int*w2;
   bzero(buffer1, 500);
   bzero(buffer2, 500);
  addr_size = sizeof(client_addr);
  recvfrom(sockfd, buffer1,500, 0, (struct sockaddr*)&client_addr, &addr_size);
  r1=atoi(buffer1);
  addr_size = sizeof(client_addr1);
  recvfrom(sockfd1, buffer2,500, 0, (struct sockaddr*)&client_addr1, &addr_size);
  r2=atoi(buffer2);
  if(r1==4 || r2==4){
    r1=4;
    r2=4;
  }else if(r1==1 && r2==1){
    r1=7;
    r2=7;
  }else if(r1==1 && r2==2){
    r1=6;
    r2=5;
  }else if(r1==1 && r2==3){
    r1=5;
    r2=6;
  }else if(r1==2 && r2==1){
    r1=5;
    r2=6;
  }else if(r1==2 && r2==2){
    r1=7;
    r2=7;
  }else if(r1==2 && r2==3){
    r1=6;
    r2=5;
  }else if(r1==3 && r2==1){
    r1=6;
    r2=5;
  }else if(r1==3 && r2==2){
    r1=5;
    r2=6;
  }else if(r1==3 && r2==3){
    r1=7;
    r2=7;
  }else{
    r1=20;
    r2=20;
  }
  
bzero(buffer1,500);
bzero(buffer2,500);
sprintf(buffer1,"%d",r1);
sendto(sockfd, buffer1, 500,0, (struct sockaddr*)&client_addr, sizeof(client_addr));
sprintf(buffer2,"%d",r2);
sendto(sockfd1, buffer2, 500,0, (struct sockaddr*)&client_addr1, sizeof(client_addr1));
  if(r1==4 || r2==4){
    printf("Game quit by players\n");
    break;
  }
}
  close(sockfd);
  printf("[+]Client1 disconnected.\n");
  close(sockfd1);
  printf("[+]Client2 disconnected.\n");
  return 0;
}