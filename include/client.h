#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT 4000
#define DEFAULT_NICK_SIZE 20
#define DEFAULT_BUFFER_SIZE 1024

int main(int argc, char* argv[]);

void *server(void *arg);
int str_starts_with(const char *prefix, const char *str);

int sockfd;
struct hostent *host;
struct sockaddr_in serv_addr;

pthread_t reader;
pthread_mutex_t client_m = PTHREAD_MUTEX_INITIALIZER;

#endif
