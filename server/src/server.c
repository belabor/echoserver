#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <ctype.h>
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

#define MAX_BUFF 100 /* MUST BE <128 */

/* simple time stamping */
char *timeStamp() {
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


void createClientID(char *ID) {
  memset(ID, 0, 16); /* clear out the char array first */

  /* pu trandom string in front */
  srand((unsigned int)time(NULL));
  snprintf(ID, 11, "%dclient", rand() % 10000);


  /* created abbreviated timestamp */
  char *ats = (char *)malloc(sizeof(char) * 6);
  time_t ltime;
  ltime=time(NULL);
  struct tm *tm;
  tm=localtime(&ltime);

  sprintf(ats,"%02d%02d%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);

  /*add timestamp for connect */
  int i=0;
  for (i = 10; i <= 16; i++) {
    ID[i] = ats[i-10];
  }
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



int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
  unsigned char *iv, unsigned char *ciphertext)
{
  EVP_CIPHER_CTX *ctx;

  int len;

  int ciphertext_len;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new()))
	{
	print_to_console("Error E1", 8);
	}
  /* Initialise the encryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    {
	print_to_console("Error E2", 8);
	}
  /* Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
    {
	print_to_console("Error E3", 8);
	}
	ciphertext_len = len;
  /* Finalise the encryption. Further ciphertext bytes may be written at
   * this stage.
   */
  if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
	{
			print_to_console("Error E4", 8);
	}
  ciphertext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
  unsigned char *iv, unsigned char *plaintext)
{

  EVP_CIPHER_CTX *ctx;

  int len;

  int plaintext_len;
  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new()))
	{
		print_to_console("Error D2", 8);
	}
  /* Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    {
		print_to_console("Error D3", 8);
	}
  /* Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    {
		print_to_console("Error D4", 8);
	}
	  plaintext_len = len;
  /* Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
  if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
	{
		print_to_console("Error D5", 8);
	}
	plaintext_len += len;
  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return plaintext_len;
}


int main(int argc, char **argv) {

  /* setup the port number */
  int portNum = 0;
  if (argc == 2) {
    portNum = (unsigned short)string_to_int(argv[1]);
    print_to_console("Error portNum: %d\n", portNum);
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

  /* Crypto variables */

      /* A 256 bit key */
      unsigned char *key = (unsigned char *)"01234567890123456789012345678901";

      /* A 128 bit IV */
      unsigned char *iv = (unsigned char *)"01234567890123456";

      /* Message to be encrypted */
      /* unsigned char *plaintext =
                  (unsigned char *)"The quick brown fox jumps over the lazy dog";
      */
      /* Buffer for ciphertext. Ensure the buffer is long enough for the
       * ciphertext which may be longer than the plaintext, dependant on the
       * algorithm and mode
       */
      unsigned char codedSendLine[128];
      unsigned char codedRecvLine[128];
      /* Buffer for the decrypted text */
      /* unsigned char decryptedtext[128]; */
      int decryptedtext_len, ciphertext_len;
      //int decryptedtext_len = 0;
      //int ciphertext_len = 0;


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
      char *clientID = (char *)malloc(sizeof(char) * 16);
      createClientID(clientID);
      print_to_console(clientID, 16);
      print_to_console(" connected.\n", 12);


      while (1) {
        /* set readLength to length of string */
        int readLength = read(comm_fd, codedRecvLine, MAX_BUFF);
        int sentLength;
        if (readLength < 0) {
          print_to_console("Error reading from client.\n", 27);
          break;
        } else if (readLength == 0) {
          print_to_console("Client has closed connection!\n", 30);
          close(comm_fd);
          break;
        } else if (readLength > 0) { /* successfully connected */

          /* Initialise the library */
          ERR_load_crypto_strings();
          OpenSSL_add_all_algorithms();
          OPENSSL_config(NULL);

          decryptedtext_len = decrypt(codedRecvLine, 16, key, iv, str);
          str[decryptedtext_len]='\0';

          /* print out to console */

          print_to_console(clientID, 16);
          print_to_console("-", 1);
          print_to_console(timeStamp(), 14);
          print_to_console(":", 1);
          print_to_console(str, decryptedtext_len);

          /* echo to client */
          /*
          sentLength = write_data(comm_fd, timeStamp(), 14);
          sentLength += write_data(comm_fd, ": ", 2);
          sentLength += write_data(comm_fd, str, readLength);
          */
          ciphertext_len = encrypt(str, decryptedtext_len, key, iv, codedSendLine);
          sentLength = write_data(comm_fd, codedSendLine, ciphertext_len);
          if (sentLength > 0) { /* Do noting, everything is ok */
          } else if (sentLength < 0) {
            print_to_console("Problem with sending to client!\n", 32);
          } else {
            print_to_console("Nothing sent to client (possible error).\n", 41);
          }

          /* log to file */
          FILE *fp;
          fp = fopen("log.txt", "a");
          fprintf(fp, "%s-%s: %s", clientID, timeStamp(), str);
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
