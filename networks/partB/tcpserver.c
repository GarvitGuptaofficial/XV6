#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>

struct datachunk{
    char buff[10];
    int seqnum;
    int numchunks;
    int chunks_sent;
};

struct ack{
    int seqnum;
};

int main(int argc, char **argv){

  if (argc != 2){
    printf("Usage: %s <port>\n", argv[0]);
    exit(0);
  }

  char *ip = "127.0.0.1";
  int port = atoi(argv[1]);

  int sockfd;
  struct sockaddr_in server_addr, client_addr;
  char buffer[1024];
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
  int max_chunks=0;
  struct datachunk data[10000];
  int flag=0;
  int*ackcheck;
  int total_recieve=0;
  int testing_count=0;

while(1){
  struct datachunk recieved_packet;
   addr_size = sizeof(client_addr);
  int l=recvfrom(sockfd, &recieved_packet, sizeof(struct datachunk), 0, (struct sockaddr*)&client_addr, &addr_size);
  if(l<0){
    printf("recvform failed\n");
    return 0;
  }
  printf("Packet with SEQ.NUMBER RECEIVED %d\n",recieved_packet.seqnum);
  if(flag==0){
    flag=1;
    max_chunks=recieved_packet.numchunks;
    ackcheck=(int*)malloc(sizeof(int)*(max_chunks));
    for(int i=0;i<max_chunks;i++){
      ackcheck[i]=0;
    }
  }

// if(testing_count%3!=0){
  ackcheck[recieved_packet.seqnum]=1;
  struct ack A;
  A.seqnum=recieved_packet.seqnum;
  int f=sendto(sockfd,&A, sizeof(struct ack), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
  if(f<0){
    printf("sendto failed\n");
    return -1;
  }
  total_recieve++;
  printf("ACK sent :%d\n",recieved_packet.seqnum);
  // printf("%s\n",recieved_packet.buff);
// }

// if(max_chunks>0 && testing_count%max_chunks==0){
//   for(int i=0;i<recieved_packet.chunks_sent;i++){
//     if(ackcheck[i]==0){
//       int z=
//     }
//   }
// }
  data[recieved_packet.seqnum]=recieved_packet;
  testing_count++;
  if(total_recieve==max_chunks){
    break;
  }
  usleep(1000000);
}

    char printdata[10*max_chunks+1];
    memset(printdata, 0, sizeof(printdata));
    for(int i=0;i<max_chunks;i++){
      strcat(printdata,data[i].buff);
    }
    printf("Data Received:%s\n",printdata);
  close(sockfd);
  return 0;
}