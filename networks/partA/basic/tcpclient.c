#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(){

  char *ip = "127.0.0.1";
  int port = 5567;

  int sock;
  struct sockaddr_in addr;
  socklen_t addr_size;
  char buffer[1024];
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

  bzero(buffer, 1024);
  scanf("%s",buffer);
  printf("Client: %s\n", buffer);
  int send_r=send(sock, buffer, strlen(buffer), 0);
  if(send_r<0){
      printf("send failed\n");
      return -1;
  }

  bzero(buffer, 1024);
  int recv_r=recv(sock, buffer, sizeof(buffer), 0);
  if(recv_r<0){
      printf("recv failed\n");
      return -1;
    }
  printf("Server: %s\n", buffer);

  printf("Disconnected from the server.\n");
  close(sock);
  return 0;

}