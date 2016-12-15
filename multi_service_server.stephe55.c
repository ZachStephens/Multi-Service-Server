#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>


#define LISTENQ 128

void echo(int);
int open_listenfd(int);
int open_pingfd(int);
int max1(int, int);

int main(int argc, char **argv) {
  int listenfd, connfd; 
  int port_http, port_ping, childpid;
  struct sockaddr_in clientaddr, clientaddr_ping;
  
  //select stuff
  int maxfd, retval;
  fd_set rfds;

  //udp stuff
  char * pingbuff[68];//32 bit header + 64 byte message
  int rcv_count, pingfd;
  socklen_t clientlen, pinglen;
  uint32_t pingnum;// 32 bit number to be psocklen_tinged

  //handle input port numbers
  if(argc < 3) {
    printf("Too few arguments\n");
    return -1;
  }
  port_http = atoi(argv[1]);
  port_ping = atoi(argv[2]);

  //obtain 
  listenfd = open_listenfd(port_http);
  pingfd = open_pingfd(port_ping);

  while (1) {

    FD_ZERO(&rfds);
    FD_SET(listenfd, &rfds);
    FD_SET(pingfd, &rfds);
    maxfd = max1(listenfd,pingfd);
    retval = select(maxfd, &rfds, NULL, NULL, NULL); 

    if(FD_ISSET(listenfd,&rfds)){
      //handle tcp
      clientlen = sizeof(clientaddr);
      connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
      if((childpid = fork())==0){
	close(listenfd);
	echo(connfd);
	exit(0);
      }
      close(connfd);
    }
    else if (FD_ISSET(pingfd,&rfds)){
      //handle udp ping
      rcv_count = recvfrom(pingfd,pingbuff,68,0,(struct sockaddr *)&clientaddr_ping,&pinglen);
      pingnum = ntohl(*(uint32_t*)pingbuff);
      *((uint32_t*)pingbuff) = htonl(++pingnum);
      sendto(pingfd,pingbuff,rcv_count,0,(struct sockaddr *)&clientaddr_ping,pinglen);
    }
  }
}

int max1(int a, int b)
{
  return ((a>b)?a:b)+1;
}

//obtains tcp descriptor
int open_listenfd(int port)
{
  int listenfd, optval=1;
  struct sockaddr_in serveraddr;

  /* Create a socket descriptor */
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -1;

  /* Eliminates "Address already in use" error from bind. */
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
		 (const void *)&optval , sizeof(int)) < 0)
    return -1;

  /* Listenfd will be an endpoint for all requests to port
     on any IP address for this host */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    return -1;
  /* Make it a listening socket ready to accept
     connection requests */
  if (listen(listenfd, LISTENQ) < 0)
    return -1;
  return listenfd;
} 


//obtains UDP descriptor
int open_pingfd(int port)
{
  int listenfd =-1; 
  int optval=1;
  struct sockaddr_in serveraddr;

  /* Create a socket descriptor */
  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    return -1;

  /* Eliminates "Address already in use" error from bind. */
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
		 (const void *)&optval , sizeof(int)) < 0)
    return -1;

  /* Listenfd will be an endpoint for all requests to port
     on any IP address for this host */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    return -1;
  return listenfd;
} 

void echo(int connfd){

  FILE * fptr;
  char input_buffer[2048];
  char * ok_str = "HTTP/1.0 200 OK\r\n\r\n";
  char * notfound_str= "HTTP/1.0 404 Not Found\r\n\r\n";
  char * forbid_str = "HTTP/1.0 403 Forbidden\r\n\r\n" ;
  char * file_pathname;
  read(connfd, input_buffer, 2048);
  file_pathname = strtok(input_buffer, " ");
  file_pathname = strtok(NULL, " "); 

  if (access(file_pathname, R_OK) == 0)
    {
      write(connfd, ok_str, strlen(ok_str));
      fptr = fopen(file_pathname, "r");
      while (!feof(fptr))
	{
	  memset(input_buffer,0,2048);
	  fread(input_buffer, sizeof(char), 2047, fptr);
	  write(connfd, input_buffer, strlen(input_buffer));
	}
      fclose(fptr);
    }
  else
    {
      if (access(file_pathname, F_OK) != 0)
	write(connfd, notfound_str, strlen(notfound_str));
      else
	write(connfd, forbid_str, strlen(forbid_str));
    } 
  return;
}
