#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_BUFF 100 /* MUST BE <128 */

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
  int portNum = 0;
  if (argc == 2) {
    portNum = (unsigned short)string_to_int(argv[1]);
    printf("portNum: %d\n", portNum);
  }
  if (portNum == 0) {
    portNum = 22000; /* default to port 22000 */
    print_to_console("Defaulting to port 22000\n", 25);
  }

  /* unsigned char *key = (unsigned char *)"thisisakeyaverynicekeyilikethis1";
*/


  int sockfd = 0;
  char sendline[MAX_BUFF];
  unsigned char recvline[MAX_BUFF];
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


      /* Buffer for ciphertext. Ensure the buffer is long enough for the
      * ciphertext which may be longer than the plaintext, dependant on the
      * algorithm and mode
      */

  unsigned char codedSendLine[128];
  unsigned char codedRecvLine[128];


  int decryptedtext_len, ciphertext_len;

  	  /* A 256 bit key */
  	  unsigned char *key = (unsigned char *)"01234567890123456789012345678901";

  	  /* A 128 bit IV */
  	  unsigned char *iv = (unsigned char *)"01234567890123456";


  while (1) {
    bzero(sendline, MAX_BUFF);
    bzero(codedSendLine, MAX_BUFF);
    bzero(recvline, MAX_BUFF);
    bzero(codedRecvLine, MAX_BUFF);

    fgets(sendline, MAX_BUFF, stdin); /*stdin = 0 , for standard input */


    /* Initialise the library */
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);


    ciphertext_len = encrypt (sendline, strlen ((char *)sendline), key, iv, codedSendLine);


    writeLength = write(sockfd, codedSendLine, ciphertext_len+1);
    if (writeLength < 0) {
      print_to_console("Error writing to socket\n", 24);
    }
    readLength = read(sockfd, codedRecvLine, ciphertext_len);
    if (readLength <= 0) {
      print_to_console("Error reading socket\n", 21);
    } else {
      decryptedtext_len = decrypt(codedRecvLine, ciphertext_len, key, iv, recvline);

      recvline[decryptedtext_len] = '\0';

      print_to_console("Server responded with: ", 23);
      print_to_console(recvline, decryptedtext_len);
      print_to_console("\n", 1);
    }
  }
}
