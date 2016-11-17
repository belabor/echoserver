#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_BUFF 100 /* max length of string */

/* simple time stamping */
char *timestamp() {
  char *stamp = (char *)malloc(sizeof(char) * 14);
  time_t ltime = time(NULL);
  struct tm *tm;
  tm = localtime(&ltime);

  sprintf(stamp, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon,
          tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
  return stamp;
}

/* convert String to an Int without atoi */
int string_to_int(const char *str) {
  int val = 0;
  while (*str) {
    val = val * 10 + (*str++ - '0');
  }
  return val;
}

/* Writes out given message to socket */
int write_data(int socket, char *buffer, int i) {
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
void print_to_console(char *buffer, int i) {
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
  if (argc == 2) {
    portNum = (unsigned short)string_to_int(argv[1]);
    printf("portNum: %d\n", portNum);
  }
  if (portNum == 0) {
    portNum = 22000; /* default to port 22000 */
    print_to_console("Defaulting to port 22000\n", 25);
  }

  /* create a TCP Stream */
  int listen_fd = 0;
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0)
    return -1;
  print_to_console("Created TCP Stream\n", 19);

  /* time to make the sockets */
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));

  print_to_console("Createing socket\n", 17);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons((unsigned short)portNum);

  /* bind the socket to servaddr */
  if (bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    close(listen_fd);
    print_to_console("Something went wrong when binding to the socket\n", 48);
    return -1;
  } else {
    print_to_console("bound to the socket\n", 20);
  }

  /* Now let's listen in.... */
  if (listen(listen_fd, 10) == -1) {
    close(listen_fd);
    print_to_console("Something went wrong when listening to port\n", 44);
    return -1;
  } else {
    print_to_console("Listening in...\n", 16);
  }

  /* create an infinite loop as they connect */
  struct sockaddr_in peer_addr;
  socklen_t addr_len = sizeof(peer_addr);
  while (1) {
    print_to_console("Loop (re)started\n", 17);
    int comm_fd = accept(listen_fd, (struct sockaddr *)&peer_addr, &addr_len);
    if (comm_fd == -1) {
      print_to_console("Socket is not established\n", 26);
      break;
    }
    pid_t pid = fork();
    if (pid < 0) {
      print_to_console("Problem with forking.\n", 22);
      break;
    }

    /* which process is this??? */
    if (pid == 0) {
      /* this is the child */
      close(listen_fd);
      char str[MAX_BUFF];
      while (1) {
        /* set readLength to length of string */
        int readLength = read(comm_fd, str, MAX_BUFF);
        int sentLength;
        if (readLength < 0) {
          print_to_console("Error reading from client.\n", 27);
          break;
        } else if (readLength == 0) {
          print_to_console("Client has closed connection!\n", 30);
          close(comm_fd);
          break;
        } else if (readLength > 0) {
          /* print out to console */
          print_to_console(timestamp(), 14);
          print_to_console(" - ", 3);
          print_to_console(str, readLength);

          /* echo to client */
          sentLength = write_data(comm_fd, str, readLength);
          if (sentLength > 0) { /* Do noting, everything is ok */
          } else if (sentLength < 0) {
            print_to_console("Problem with sending to client!\n", 32);
          } else {
            print_to_console("Nothing sent to client (possible error).\n", 41);
          }

          /* log to file */
          FILE *fp;
          fp = fopen("log.txt", "a");
          fprintf(fp, "%s - %s", timestamp(), str);
          fclose(fp);
        }
      }
    } else if (pid > 0) {
      /* This is the parent */
      close(comm_fd);
      print_to_console("Parent fork established\n", 24);
    }
  }
  print_to_console("Fork closed\n", 12);
  return 0;
}
