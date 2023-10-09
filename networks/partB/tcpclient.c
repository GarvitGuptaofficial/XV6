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
  memset(&addr, '\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip);

  char*input=(char*)malloc(sizeof(char)*10000);
  printf("Enter the Input:");
  fgets(input,10000,stdin);
  int num_chunks=(strlen(input)+9)/10;
  int ackcheck[num_chunks];
  for(int i=0;i<num_chunks;i++){
    ackcheck[i]=0;
  }
  fd_set sendfds;
  int time_passed=0;
  int chunks_sent=0;
  struct timeval timer;
  timer.tv_usec=1000000;
  timer.tv_sec=0;
  int flag=0;
  
while(1){

  for(int i=0;i<num_chunks;i++){
       if(ackcheck[i]==1){
          continue;
       }
       char*send_string=(char*)malloc(sizeof(char)*(12));
       strncpy(send_string,&input[i*10],10);
       send_string[11]='\0';
       struct datachunk*packet=(struct datachunk*)malloc(sizeof(struct datachunk)*1);
       strcpy(packet->buff,send_string);
       packet->numchunks=num_chunks;
       packet->seqnum=i;
       packet->chunks_sent=chunks_sent;
       int w=sendto(sockfd, packet, sizeof(struct datachunk), 0,(struct sockaddr*)&addr, sizeof(addr));
       if(w<0){
           printf("sendto failed\n");
           return -1;
       }
       printf("Packet with SEQ.NUMBER %d sent\n",i);
           struct ack A;
           
           int r=recvfrom(sockfd,&A, sizeof(struct ack), 0, NULL, NULL);
           if(r<=0){
            printf("Error\n");
           }
           printf("ACK received:%d\n",A.seqnum);
           ackcheck[A.seqnum]=1;
           chunks_sent++;
      //  }
       time_passed++;
   if(chunks_sent==num_chunks){
        break;
       }
  usleep(1000000);
  }
       if(chunks_sent==num_chunks){
        printf("Data has been sent\n");
        break;
       }
}
       printf("Disconnect from Server\n");
       close(sockfd);
       return 0;

  
}