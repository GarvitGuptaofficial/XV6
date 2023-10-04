#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char **argv){
  if(argc!=2){
    printf("Usage: %s <port>\n", argv[0]);
    exit(0);
  }

  char *ip = "127.0.0.1";
  int port = atoi(argv[1]);

  int sock;
  struct sockaddr_in addr;
  socklen_t addr_size;
  char buffer[500];
  int n;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0){
    perror("[-]Socket error");
    exit(1);
  }
  printf("[+]TCP server socket created.\n");
  memset(&buffer,'\0',sizeof(buffer));
  memset(&addr, '\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip);
  int connect_r=connect(sock, (struct sockaddr*)&addr, sizeof(addr));
  if(connect_r<0){
    printf("Connect failed\n");
    return -1;
  }
  printf("Connected to the server.\n");
while(1){
  bzero(buffer, 500);
  int r;
    printf("1 for rock\n");
    printf("2 for paper\n");
    printf("3 for scissor\n");
    printf("4 to Exit Game\n");
    printf("Player1 Enter your choice:");
    scanf("%d",&r);
  sprintf(buffer,"%d",r);
  int send_r=send(sock, buffer, strlen(buffer), 0);
  if(send_r<0){
      printf("send failed\n");
      return -1;
  }

  bzero(buffer, 500);
  int recv_r=recv(sock, buffer, sizeof(buffer), 0);
  if(recv_r<0){
      printf("recv failed\n");
      return -1;
    }
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
  printf("Disconnected from the server.\n");
  close(sock);
  return 0;

}