#include <stdlib.h>
#include <stdio.h>
#include "csapp.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
//#include "csapp.c"
#include "sbuf.c"



#define BACKLOG 10
#define DEFAULT_PORT_STR "22222"

#define EXIT_USAGE_ERROR 1
#define EXIT_GETADDRINFO_ERROR 2
#define EXIT_BIND_FAILURE 3
#define EXIT_LISTEN_FAILURE 4

#define MAX_LINE 128
#define MAX_ENTRIES 120000 //maximum dictionary size of 100,000 items


int getlistenfd(char*); //functions declared later
int readDict(char *fileName, char *dictionary[]);
void *worker();
void lookup(int clientsd);

char **dictionary; //global variables
pthread_t threadIDs[20];
int no_entries;

sbuf_t sbuf; //shared buffer of connected descriptors

int main(int argc, char **argv) {

  int i = 0;
  char *port = DEFAULT_PORT_STR; //setting port to default name, will check & change in a few lines
  char *dictName = "dict.txt"; //dictionary name, will get from arguments eventually
  sbuf_init(&sbuf, 100); //initialize a buffered queue of size 100 based on Bryant & O'Halloran

  dictionary = malloc(sizeof(char *) * MAX_ENTRIES); //allocate dictionary size based on defined max
  no_entries = readDict(dictName, dictionary); //how many words were added


  for(i = 0; i < 20; i++){ //creates 20 worker threads
    if(pthread_create(&threadIDs[i], NULL, worker, NULL) != 0){
      printf("error creating %d. \n", i);
      return(EXIT_FAILURE);
    }
  }


  int listenfd,	       /* listen socket descriptor */
    clientsd;       /* connected socket descriptor */
  struct sockaddr_storage client_addr;
  socklen_t client_addr_size;
  char client_name[MAX_LINE];
  char client_port[MAX_LINE];

  listenfd = getlistenfd(port); //listenfd is the server's fd


  while(1) {     
      
    client_addr_size=sizeof(client_addr);
    if ((clientsd=accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_size))==-1) {
      fprintf(stderr, "accept error\n");
      continue;
    }
    if (getnameinfo((struct sockaddr*)&client_addr, client_addr_size,
		    client_name, MAX_LINE, client_port, MAX_LINE, 0)!=0) {
      fprintf(stderr, "error getting name information about client\n");
    } else {
      printf("accepted connection from %s:%s\n", client_name, client_port);
    }
    sbuf_insert(&sbuf, clientsd); //insert into buffer, wakes someone up, is protected

  }
  return 0; //since this is an infinite loop, this will probably never return unless error
}

/* given a port number or service as string, returns a
   descriptor that we can pass to accept() */
int getlistenfd(char *port) {

  int listenfd, status;
  struct addrinfo hints, *res, *p;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM; /* TCP */
  hints.ai_family = AF_INET;     /* IPv4 */

  if ((status = getaddrinfo(NULL, port, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo error %s\n", gai_strerror(status));
    exit(EXIT_GETADDRINFO_ERROR);
  }

   // try to bind to the first available address/port in the list.
   //   if we fail, try the next one. 
  for(p = res;p != NULL; p = p->ai_next) {
    if ((listenfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))<0) {
      continue;
    }

    if (bind(listenfd, p->ai_addr, p->ai_addrlen)==0) {
      printf("socket created\n");
      break;
    }
  }
  freeaddrinfo(res);
  if (p==NULL) {
    exit(EXIT_BIND_FAILURE);
  }

  if (listen(listenfd, BACKLOG)<0) {
    close(listenfd);
    exit(EXIT_LISTEN_FAILURE);
  }
  return listenfd;

}

int readDict(char *fileName, char *dictionary[]){ //uploads dictionary to an array
  FILE *fp = fopen(fileName, "r"); //open the dictionary file passed in read mode
  char* word = NULL;
  int count = 0;
  size_t nbytes = 0;

  while((getline(&word, &nbytes, fp)) != -1){
    strtok(word, "\n"); //get rid of the new line char at the end of every word
    dictionary[count] = malloc((strlen(word) + 1) * sizeof(char)); //allocate space for the word
    strcpy(dictionary[count], word);
    count++;
  }
  fclose(fp);
  if(word)
    free(word);
  

  return count;
}

void *worker(){

  while(1){
    int clientsd = sbuf_remove(&sbuf);
    lookup(clientsd);
    close(clientsd);
  }
}

void lookup(int clientsd){

  ssize_t bytes_read;
  char line[MAX_LINE];
  int i;
  int n;
  int isWord = 0;
  char *ok = "OK\n";
  char *no = "MISSPELLED\n";

  memset(line, '\0', sizeof(line)); //sets everything to null character, soon to be overwritten

  while((n = read(clientsd, line, MAX_LINE)) != 0){

    for(i = 0; i < no_entries; i++){
      if(strcmp(dictionary[i], line) == 0){
        isWord = 1;
        break;
      }
    }
    if(isWord){
      write(clientsd, ok, strlen(ok) + 1);
    }
    else{
      write(clientsd, no, strlen(no) + 1);
    }
  }
}
