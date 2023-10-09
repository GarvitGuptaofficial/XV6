#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char **argv){
   if(argc!=3){
    printf("Usage: %s <port>\n", argv[0]);
    exit(0);
   }
  char *ip = "127.0.0.1";
  int port1 = atoi(argv[1]);
  int port2 = atoi(argv[2]);
  int server_sock1, client_sock1,server_sock2, client_sock2;
  struct sockaddr_in server_addr1, client_addr1,server_addr2, client_addr2;
  socklen_t addr_size1,addr_size2;
  char buffer1[500];
  char buffer2[500];
  int n;

  server_sock1 = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock1 < 0){
    perror("[-]Socket error");
    exit(1);
  }
  printf("[+]TCP server socket1 created.\n");

  memset(&server_addr1, '\0', sizeof(server_addr1));
  server_addr1.sin_family = AF_INET;
  server_addr1.sin_port = htons(port1);
  server_addr1.sin_addr.s_addr = inet_addr(ip);

  n = bind(server_sock1, (struct sockaddr*)&server_addr1, sizeof(server_addr1));
  if (n < 0){
    perror("[-]Bind error");
    exit(1);
  }
  printf("[+]Bind to the port number: %d\n", port1);

  if(listen(server_sock1, 5)<0){
     printf("Error performing listen\n");
     return -1;
  }
//   printf("Listening...\n");

    addr_size1 = sizeof(client_addr1);
    client_sock1 = accept(server_sock1, (struct sockaddr*)&client_addr1, &addr_size1);
    if(client_sock1<0){
      printf("accept failed\n");
      return -1;
    }
  printf("[+]Client1 connected.\n");

  server_sock2 = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock2 < 0){
    perror("[-]Socket error");
    exit(1);
  }
  printf("[+]TCP server socket2 created.\n");

  memset(&server_addr2, '\0', sizeof(server_addr2));
  server_addr2.sin_family = AF_INET;
  server_addr2.sin_port = htons(port2);
  server_addr2.sin_addr.s_addr = inet_addr(ip);

  n = bind(server_sock2, (struct sockaddr*)&server_addr2, sizeof(server_addr2));
  if (n < 0){
    perror("[-]Bind error");
    exit(1);
  }
  printf("[+]Bind to the port number: %d\n", port2);

  if(listen(server_sock2, 5)<0){
     printf("Error performing listen\n");
     return -1;
  }
//   printf("Listening...\n");

    addr_size2 = sizeof(client_addr2);
    client_sock2 = accept(server_sock2, (struct sockaddr*)&client_addr2, &addr_size2);
    if(client_sock2<0){
      printf("accept failed\n");
      return -1;
    }
    printf("[+]Client2 connected.\n");

  while(1){
    int r1,r2;
    
    bzero(buffer1, 500);
    int recv_r1=recv(client_sock1, buffer1, sizeof(buffer1), 0);
    if(recv_r1<0){
      printf("recv failed\n");
      return -1;
    }
    r1=atoi(buffer1);
    
    bzero(buffer2, 500);
    int recv_r2=recv(client_sock2, buffer2, sizeof(buffer2), 0);
    if(recv_r2<0){
      printf("recv failed\n");
      return -1;
    }
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
  
    bzero(buffer1, 500);
    sprintf(buffer1,"%d",r1);
    int send_r1=send(client_sock1, buffer1, strlen(buffer1), 0);
    if(send_r1<0){
      printf("send failed\n");
      return -1;
    }
    
    bzero(buffer2, 500);
    sprintf(buffer2,"%d",r2);
    int send_r2=send(client_sock2, buffer2, strlen(buffer2), 0);
    if(send_r2<0){
      printf("send failed\n");
      return -1;
    }

    if(r1==4 || r2==4){
        printf("Game quit by players\n");
        break;
    }

  }
    close(client_sock1);
    printf("[+]Client1 disconnected.\n");
    close(client_sock2);
    printf("[+]Client2 disconnected.\n");
  return 0;
}