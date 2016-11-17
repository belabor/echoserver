#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_BUFF 100

/*writes given message to local console */
void print_to_console(char *buffer, int i) {
  while (i > 0) {
    char c = *buffer;
    fprintf(stderr, "%c", c);
    buffer++;
    i--;
  }
}

/* convert String to an Int without atoi */
int string_to_int(const char *str) {
  int val = 0;
  while (*str) {
    val = val * 10 + (*str++ - '0');
  }
  return val;
}

int main(int argc, char **argv) {
  int portNum = 0;
  if (argc == 2) {
    portNum = (unsigned short)string_to_int(argv[1]);
    printf("portNum: %d\n", portNum);
  }
  if (portNum == 0) {
    portNum = 22000; /* default to port 22000 */
    print_to_console("Defaulting to port 22000\n", 25);
  }

  int sockfd = 0;
  char sendline[MAX_BUFF];
  char recvline[MAX_BUFF];
  struct sockaddr_in servaddr;
  int readLength = 0;
  int writeLength = 0;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero(&servaddr, sizeof servaddr);

  servaddr.sin_family = AF_INET;

  servaddr.sin_port = htons(portNum);

  /* This should probably ask for a specific address later */
  inet_pton(AF_INET, "127.0.0.1", &(servaddr.sin_addr));

  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    print_to_console("Error establishing connection\n", 30);
  };

  while (1) {
    bzero(sendline, MAX_BUFF);
    bzero(recvline, MAX_BUFF);
    fgets(sendline, MAX_BUFF, stdin); /*stdin = 0 , for standard input */

    writeLength = write(sockfd, sendline, strlen(sendline) + 1);
    if (writeLength < 0) {
      print_to_console("Error writing to socket\n", 24);
    }
    readLength = read(sockfd, recvline, MAX_BUFF);
    if (readLength < 0) {
      print_to_console("Error reading socket\n", 21);
    } else {
      print_to_console("Server responded with: ", 23);
      print_to_console(recvline, readLength);
    }
  }
}
