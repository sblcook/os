#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "22222"
#define EXIT_GETADDRINFO_FAILURE 1
#define EXIT_CONNECT_FAILURE 2

#define MAX_LINE 128
#define CONNECT_STR_MAX 256

int getclientsd(char*,char*);

int main(int argc, char *argv[]) {

  size_t bytes_read;
  int sd;			/* socket descriptor */
  char line[MAX_LINE];
  int status;
  char *host;
  char connect_str[CONNECT_STR_MAX];
  char word[MAX_LINE];
  char result[MAX_LINE];

  if (argc != 2) {
    fprintf(stderr,"usage: %s host\n", argv[0]);
    exit(1);
  }

  host=argv[1]; //sets IP address to 1st argument

  sd = getclientsd(host, PORT); //connects

  while(1){

    memset(word, '\0', MAX_LINE); //clears words
    memset(result, '\0', MAX_LINE);
    
    fgets(word, MAX_LINE, stdin); //reads word from user
    strtok(word, "\n"); //makes sure theres no \n at the end of the word
    write(sd, word, MAX_LINE + 1); //sends it to the server
    read(sd, result, MAX_LINE); //reads result

    printf("%s is %s", word, result); 

  }

  close(sd);
  return 0;
}
int getclientsd(char *host, char *port) {
  int status;
  int sd;			/* socket descriptor */
  struct addrinfo hints, *p, *servinfo;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return EXIT_GETADDRINFO_FAILURE;
  }

  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sd = socket(p->ai_family, p->ai_socktype,
			 p->ai_protocol)) == -1) {
      continue;
    }

    if (connect(sd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sd);
      continue;
    }
    break;
  }

  if (!p) {
    fprintf(stderr, "failed to connect to %s\n", host);
    exit(EXIT_CONNECT_FAILURE);
  }

  freeaddrinfo(servinfo);
  return sd;
}
