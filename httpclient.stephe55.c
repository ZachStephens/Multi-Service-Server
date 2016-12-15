#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>


int ConnectAndDescriptor(char*,int);


int main(int argc, char** argv){
  
  char* server_name;
  int port_number;
  char* file_pathname;
  char* get_command;
  int socket_descriptor = -1;
  char InputBuffer[2048];

  //read arguments in 
  if(argc < 4){
    printf("Too few arguments\n");
    return 1;
  }

  //1. server name_name
  server_name = argv[1];
  //2. server port
  port_number = atoi(argv[2]);
  //3. pathname
  file_pathname = argv[3];

  //construct get command as string
  char * get_string = "GET ";
  char * http_string = " HTTP/1.0\r\n\r\n";
  get_command = malloc(strlen(get_string)+strlen(http_string)+strlen(file_pathname)+1);
  memcpy(get_command,get_string,strlen(get_string)+1);
  get_command = strcat(get_command,file_pathname);
  get_command = strcat(get_command,http_string);
  //printf("%s:%d",get_command,strlen(get_command)); 

  //open connection to server; obtain socket descriptor
  socket_descriptor = ConnectAndDescriptor(server_name,port_number);
  if(socket_descriptor < 0){
    printf("Failed connection\n");
    return 1;
  }
  
  write(socket_descriptor,get_command,strlen(get_command));
 
  do{
    memset(InputBuffer,0,2048);
    read(socket_descriptor,InputBuffer,2047);
    printf("%s",InputBuffer);

  }while(strlen(InputBuffer)>0);
  free(get_command);
  return 0;
}

int ConnectAndDescriptor(char* server_name,int port_number)
{
  int socket_descriptor;
  struct hostent *hostptr;
  struct sockaddr_in sock_addr;
  
  if((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0))<0)
    return -1;
  
  bzero(&sock_addr,sizeof(struct sockaddr_in));
  sock_addr.sin_family = AF_INET;
  
  if((hostptr = gethostbyname(server_name))== NULL)
    return -2;

  sock_addr.sin_addr.s_addr = *(int *)hostptr->h_addr;
  bzero((char *) &sock_addr, sizeof(sock_addr));
  sock_addr.sin_family = AF_INET;
  bcopy((char *)hostptr->h_addr,(char *)&sock_addr.sin_addr.s_addr, hostptr->h_length);
  sock_addr.sin_port = htons(port_number);

  if(connect(socket_descriptor, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in))<0)
    return -1;
  return socket_descriptor;
}
