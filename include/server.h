#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 4000

#define PVT_ROOM_SIZE 2
#define DEFAULT_ROOM_SIZE 50
#define DEFAULT_BUFFER_SIZE 1024

#define PVT_ROOM "pvt_"
#define MAIN_ROOM "lobby"
#define PVT_REQUEST "This is a request for a private conversation. Answer '/yes' to accept, and '/no' to decline."

#define NUM_COLORS (sizeof(user_colors) / sizeof(color_black))

int current_color = 0;
char *color_red = "\033[0;31m";
char *color_black = "\033[0;30m";
char *user_colors[] = {
	"\033[0;32m",
	"\033[0;33m",
	"\033[0;34m",
	"\033[0;35m",
	"\033[0;36m",
	"\033[1;30m",
	"\033[1;31m",
	"\033[1;32m",
	"\033[1;33m",
	"\033[1;34m",
	"\033[1;35m",
	"\033[1;36m"
};

typedef struct room_t
{
	char *name;
	int n_users;
	int max_users;
	char *pvt_receiver_nick;
	struct client_t *pvt_sender;
	struct room_t *next;
	struct client_t **users;
} room_t;

typedef struct client_t
{
	char *name;
	char *color;
	struct room_t *room;
	struct client_t *next;
	int socket;
	int socket_len;
	struct sockaddr_in *socket_addr;
	pthread_t *thread;
} client_t;

int main(int argc, char* argv[]);

void *client(void *arg);
void exit_chat(client_t *user);
void delete_room(room_t *room);
void list_rooms(client_t *user);
void list_users(client_t *user);
void leave_room(client_t *user);
void help_client(client_t *user);
void join_room(client_t *user, char *name);
void create_room(client_t *user, char *name, int max_users); // Cria sala

int was_user_pvt_requested(char *nick);
void join_pvt_room(client_t *user);
void delete_pvt_room(client_t *user);
void create_pvt_room(client_t *pvt_sndr, char *name, char *receiver_nick);

void reply(client_t *user, char* msg);
void reply_all(client_t *user, char* msg);
void send_message(client_t *user, char *msg);
void send_message_to_user(client_t *user, char *msg, char *nick);

int str_starts_with(const char *prefix, const char *str);
int str_starts_with_nick(client_t *user, const char *str);

int is_nick_in_room(client_t *user, const char *nick);
char* get_nick_from_command(client_t *user, const char *str);

int sockfd;
struct sockaddr_in serv_addr;

room_t *rooms;
client_t *users;

pthread_mutex_t room_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_m = PTHREAD_MUTEX_INITIALIZER;

#endif
