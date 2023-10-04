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
  char buffer[500];
  socklen_t addr_size;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  memset(&addr, '\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip);
while(1){
    int r;
    printf("1 for rock\n");
    printf("2 for paper\n");
    printf("3 for scissor\n");
    printf("4 to Exit Game\n");
    printf("Player1 Enter your choice:");
    scanf("%d",&r);
  bzero(buffer, 500);
  sprintf(buffer,"%d",r);
  sendto(sockfd, buffer, 500, 0, (struct sockaddr*)&addr, sizeof(addr));
  bzero(buffer, 500);
  addr_size = sizeof(addr);
  recvfrom(sockfd, buffer,500, 0, (struct sockaddr*)&addr, &addr_size);
  r=atoi(buffer);
  if(r==5){
    printf("You(Player1) Wins\n");
  }else if(r==6){
    printf("You(Player1) Lose\n");
  }else if(r==7){
    printf("Draw\n");
  }else if(r==4){
    printf("Game Quit\n");
    break;
  }
}
  return 0;
}