#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFF 100

int main(int argc,char **argv)
{
  int portNum = 0;
  if ( argc != 2 ) {
    portNum = 22000; /* default to port 22000 */
  } else {
    portNum = atoi(argv[1]);
  }
  if (argc != 2) /* test for port number */
  {
    fprintf(stderr, "You must choose a server port.\n");
    exit(1);
  }

    int sockfd = 0;
    char sendline[MAX_BUFF];
    char recvline[MAX_BUFF];
    struct sockaddr_in servaddr;

    sockfd=socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof servaddr);

    servaddr.sin_family=AF_INET;

    servaddr.sin_port=htons(portNum);

    inet_pton(AF_INET,"127.0.0.1",&(servaddr.sin_addr));

    connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

    while(1)
    {
        bzero( sendline, MAX_BUFF);
        bzero( recvline, MAX_BUFF);
        fgets(sendline,MAX_BUFF,stdin); /*stdin = 0 , for standard input */

        write(sockfd,sendline,strlen(sendline)+1);
        read(sockfd,recvline,MAX_BUFF);
        printf("%s",recvline);
    }

}
