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

#define LISTENQ 128

void echo(int);

int main(int argc, char **argv) {
  int listenfd, connfd, port, clientlen, childpid;
  struct sockaddr_in clientaddr;
  if(argc < 2) {
    printf("Too few arguments\n");
    return -1;
  }
  port = atoi(argv[1]); /* the server listens on a port passed
			   on the command line */
  listenfd = open_listenfd(port);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
    if((childpid = fork())==0){
      close(listenfd);
      echo(connfd);
      exit(0);
    }
    close(connfd);
  }
}

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
