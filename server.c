#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_BUFF 100 /* max length of string */

/* error handler using perror */
void error_exit(char *msg) {
  perror(msg);
  exit(1);
}

/* simple time stamping */
char *timestamp() {
  char *stamp = (char *)malloc(sizeof(char) * 16);
  time_t ltime;
  ltime=time(NULL);
  struct tm *tm;
  tm=localtime(&ltime);

  sprintf(stamp,"%04d%02d%02d%02d%02d%02d", tm->tm_year+1900, tm->tm_mon,
  tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
  return stamp;
}

/* Writes out given message to socket */
int write_data(int socket, char* buffer, int i) {
  int output, total = 0;
  while (total < i) {
    output = write(socket, buffer, i - total);
    if (output == 0) {
      return total;
    }
    if (output == -1) {
      return -1;
    }
    total += output;
    buffer += output;
  }
  return total;
}

/*writes given message to local console */
void print_to_console(char* buffer, int i) {
  while (i > 0) {
    char c = *buffer;
    fprintf(stderr, "%c", c);
    buffer++;
    i--;
  }
}

int main(int argc, char **argv) {

  /* setup the port number */
  int portNum = 0;
  if ( argc <= 2 ) {
    portNum = 22000;  /* default to port 22000 */
    print_to_console("Defaulting to port 22000\n", 25);
  } else {
    portNum = atoi(argv[1]);
  }

  /* create a TCP Stream */
  int listen_fd = 0;
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) return -1;
  printf("Created TCP Stream\n");

  /* time to make the sockets */
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  printf("Createing socket\n");

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons((unsigned short)portNum);
  printf("Established!\n");

  /* bind the socket to servaddr */
  if (bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
    close(listen_fd);
    print_to_console("Something went wrong when binding to the socket\n", 48);
    return -1;
  }
  printf("bound to the socket\n");

  /* Now let's listen in.... */
  if (listen(listen_fd, 10) == -1) {
    close(listen_fd);
    print_to_console("Something went wrong when listening to port\n", 44);
    return -1;
  }
  printf("Listening in...\n");

  /* create an infinite loop as they connect */
  struct sockaddr_in peer_addr;
  socklen_t addr_len = sizeof(peer_addr);
  while(1) {
    printf("Look (re)started\n");
    int comm_fd = accept(listen_fd, (struct sockaddr*) &peer_addr, &addr_len);
    if (comm_fd == -1) {
      error_exit("Problem with accepting connections on socket\n");
    }
    pid_t pid = fork();
    if (pid == -1) {
      error_exit("Problem with forking.\n");
    }

    /* which process is this??? */
    if (pid == 0) {
      /* this is the child */
      close(listen_fd);
      char str[MAX_BUFF];
      while (1) {
        /* set readLength to length of string */
        int readLength = read(comm_fd,str,MAX_BUFF);
        if ( readLength == -1 ) {
          fprintf(stderr, "Error reading from client: %s\n", strerror(errno));
          exit(-1);
        } else if (readLength == 0) {
          fprintf(stderr, "Client has closed connection!\n");
          close(comm_fd);
          exit(0);
        } else if (readLength > 0) {
          /* print out to console */
          print_to_console(timestamp(), 16);
          print_to_console(" - ", 3);
          print_to_console(str, readLength);

          /* echo to client */
          write_data(comm_fd, str, readLength);

          /* log to file */
          FILE *fp;
          fp = fopen("log.txt","a");
          fprintf(fp, "%s - %s", timestamp(), str);
          fclose(fp);
        }
      }
    } else if (pid > 0) {
      /* This is the parent */
      close(comm_fd);
    }
  }
  return 0;
}
