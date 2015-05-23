#include "../include/server.h"

int main(int argc, char* argv[])
{
	puts("Server starting...");
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		puts("There was an Error opening socket.");
		exit(-1);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);

	if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		puts("There was Error on binding socket.");
		exit(-1);
	}

	users = NULL;
	rooms = NULL;

	listen(sockfd, 5);
	puts("Listening...");
	create_room(NULL, MAIN_ROOM, DEFAULT_ROOM_SIZE);

	char buffer[DEFAULT_BUFFER_SIZE];
	while (1)
	{
		client_t *user = malloc(sizeof(client_t));
		user->thread = malloc(sizeof(pthread_t));
		user->socket_addr = malloc(sizeof(struct sockaddr_in));
		user->socket_len = sizeof(struct sockaddr_in);

		puts("Waiting client...");
		if ((user->socket = accept(sockfd, (struct sockaddr*)user->socket_addr, &user->socket_len)) == -1)
		{
			puts("There was an Error accepting connection.");
			exit(-1);
		}
		puts("Client connected!");

		pthread_mutex_lock(&client_m);
		{
			user->next = users;
			users = user;
			bzero(buffer, DEFAULT_BUFFER_SIZE);
			read(user->socket, buffer, DEFAULT_BUFFER_SIZE);
			user->name = malloc(strlen(buffer) + 1);
			user->color = user_colors[current_color];
			current_color = (current_color + 1) % NUM_COLORS;
			strcpy(user->name, buffer);
		}
		pthread_mutex_unlock(&client_m);

		join_room(user, MAIN_ROOM);
		pthread_create(user->thread, NULL, client, (void*)user);
	}

	return 0;
}

int is_str_empty(const char* str) // Checa se uma string esta vazia, se sim retorna 1, senao 0
{
	int i = 0;
	int empty = 1;

	do
	{
		if (str != NULL && str[i] != ' ')
		{
			empty = 0; // Quando nao esta vazio
		}
		i++;
	}
	while (str != NULL && str[i] != '\0');

	return empty;
}

int str_starts_with(const char *prefix, const char *str) // Checa se a string string str ´e igual a prefix, se for retorna 1
{
	int starts_with;

	starts_with = strncmp(prefix, str, strlen(prefix)); //comparacao
	return starts_with == 0;
}

int str_starts_with_nick(client_t *user, const char *str) // Checa se a string ´e igual a algum nome dos clientes daquela sala, se sim retorna 1
{
	room_t *room = user->room;
	int i;

	for (i = 0; i < room->max_users; i++)
	{
		if (room->users[i] != NULL && room->users[i] != user)
		{
			char prefix[DEFAULT_BUFFER_SIZE] = "";
			strcat(prefix, "/");
			strcat(prefix, room->users[i]->name);
			strcat(prefix, " ");
			int starts_with = strncmp(prefix, str, strlen(prefix)); //Compara todos os nomes de clientes da sala tirando o do proprio chamador
			if (starts_with == 0)
			{
				return 1;
			}
		}
	}
	return 0;
}

void reply(client_t *user, char* msg) // Envia mensagem do servidor para o cliente em especifico
{
	char buffer[DEFAULT_BUFFER_SIZE];

	bzero(buffer, DEFAULT_BUFFER_SIZE);
	int offset = 15;
	sprintf(buffer, "%s[Administrator@%s]: %s%s", color_red, user->room->name, msg, user->color);
	int n = write(user->socket, buffer, sizeof(buffer));
	if (n < 0)
	{
		printf("There was an Error writing to socket.");
	}
}

void reply_all(client_t *user, char* msg) // Envia mensagem do servidor para todos os clientes daquela sala
{
	room_t *room = user->room;

	int offset = 15;
	char buffer[DEFAULT_BUFFER_SIZE];

	sprintf(buffer, "%s[Administrator@%s]: %s%s", color_red, user->room->name, msg, user->color);

	int i;
	pthread_mutex_lock(&client_m);
	{
		for (i = 0; i < room->max_users; i++)
		{
			if (room->users[i] != NULL)
			{
				bzero(buffer, DEFAULT_BUFFER_SIZE);
				snprintf(buffer, DEFAULT_BUFFER_SIZE, "%s[Administrator@%s]: %s%s", color_red, room->name, msg, room->users[i]->color); //mensagem do SRV vai para todos os clientes da sala
				write(room->users[i]->socket, buffer, sizeof(buffer));
			}
		}
	}
	pthread_mutex_unlock(&client_m);
}

void send_message_to_all(client_t *user, char *msg) // Envia mensagem do cliente para todos os presentes naquela sala
{
	room_t *room = user->room;
	char buffer[DEFAULT_BUFFER_SIZE];

	int i;

	pthread_mutex_lock(&client_m);
	{
		for (i = 0; i < room->max_users; i++)
		{
			if (room->users[i] != NULL && room->users[i] != user)
			{
				bzero(buffer, DEFAULT_BUFFER_SIZE);
				snprintf(buffer, DEFAULT_BUFFER_SIZE, "%s[%s@%s]: %s%s", user->color, user->name, room->name, msg, room->users[i]->color);
				write(room->users[i]->socket, buffer, sizeof(buffer));
			}
		}
	}
	pthread_mutex_unlock(&client_m);
}

void send_message_to_user(client_t *user, char *msg, char *nick) // Envia mensagem para usuario especifico se ele estiver presente na sala
{
	room_t *room = user->room;
	char buffer[DEFAULT_BUFFER_SIZE];

	int i;

	pthread_mutex_lock(&client_m);
	{
		for (i = 0; i < room->max_users; i++)
		{
			if (room->users[i] != NULL && room->users[i] != user && (strcmp(room->users[i]->name, nick) == 0))
			{
				bzero(buffer, DEFAULT_BUFFER_SIZE);
				snprintf(buffer, DEFAULT_BUFFER_SIZE, "%s[%s->%s@%s]: %s%s", user->color, user->name,  room->users[i]->name, room->name, msg, room->users[i]->color);
				write(room->users[i]->socket, buffer, sizeof(buffer));
			}
		}
	}
	pthread_mutex_unlock(&client_m);
}

void help_user(client_t *user) // Emite lista de comandos ao cliente
{
	char buffer[DEFAULT_BUFFER_SIZE];
	int offset = 0;
	room_t *it;

	offset += sprintf(buffer + offset, "\n%s\n", "Client command list");
	offset += sprintf(buffer + offset, "%s\n", "============================");
	offset += sprintf(buffer + offset, "%s\n", "   /help                -  Lists all client commands.");
	offset += sprintf(buffer + offset, "%s\n", "   /list_rooms          -  Lists all existing rooms.");
	offset += sprintf(buffer + offset, "%s\n", "   /list_users          -  Lists all users in the room.");
	offset += sprintf(buffer + offset, "%s\n", "   /join <room_name>    -  Leaves current room and joing room with given name.");
	offset += sprintf(buffer + offset, "%s\n", "   /create <room_name>  -  Creates room with the given name, for up to 50 users.");
	offset += sprintf(buffer + offset, "%s\n", "   /nick <new_nickname> -  Changes current nickname to given nickname.");
	offset += sprintf(buffer + offset, "%s\n", "   /<nickname> <msg>    -  Sends private massege to user with given nickname.");
	offset += sprintf(buffer + offset, "%s\n", "   /pvt <nickname>      -  Sends a request for a private conversation to given user.");
	offset += sprintf(buffer + offset, "%s\n", "   /leave               -  Leaves current room and goes back to the Lobby.");
	offset += sprintf(buffer + offset, "%s\n", "   /exit                -  Exits chat.");
	offset += sprintf(buffer + offset, "%s", "============================");

	reply(user, buffer);
}

void list_rooms(client_t *user) // Lista todas as salas disponiveis
{
	char buffer[DEFAULT_BUFFER_SIZE];
	int offset = 0;
	room_t *it;

	offset += sprintf(buffer + offset, "%s\n", "Room list");
	offset += sprintf(buffer + offset, "%s\n", "============================");

	pthread_mutex_lock(&room_m);
	{
		for (it = rooms; it != NULL; it = it->next)
		{
			offset += sprintf(buffer + offset, "%s [%d/%d]\n", it->name, it->n_users, it->max_users);
		}
	}
	pthread_mutex_unlock(&room_m);

	offset += sprintf(buffer + offset, "%s", "============================");
	reply(user, buffer);
}

void list_users(client_t *user) //Lista todos os usuarios da sala
{
	room_t *room = user->room;
	char buffer[DEFAULT_BUFFER_SIZE];
	int offset = 0;

	offset += sprintf(buffer + offset, "%s %s [%d/%d]\n", "Users in room", room->name, room->n_users, room->max_users);
	offset += sprintf(buffer + offset, "%s\n", "============================");

	int i;
	pthread_mutex_lock(&client_m);
	{
		for (i = 0; i < room->max_users; i++)
		{
			if (room->users[i] != NULL)
			{
				offset += sprintf(buffer + offset, "[%s%s%s]\n", room->users[i]->color, room->users[i]->name, color_red);
			}
		}
	}
	pthread_mutex_unlock(&client_m);

	offset += sprintf(buffer + offset, "%s", "============================");
	reply(user, buffer);
}

void create_room(client_t *user, char *name, int max_users) // Cria sala
{
	room_t *room = malloc(sizeof(room_t));

	room->name = malloc(strlen(name) + 1);
	strcpy(room->name, name);
	room->max_users = max_users;
	room->n_users = 0;
	room->pvt_sender = NULL;
	room->pvt_receiver_nick = NULL;
	room->users = malloc(sizeof(client_t *) * room->max_users);
	int i;
	for (i = 0; i < room->max_users; i++)
	{
		room->users[i] = NULL;
	}

	room->next = NULL;

	pthread_mutex_lock(&room_m);
	{
		room_t *it;
		for (it = rooms; it != NULL; it = it->next)
		{
			if (strcmp(it->name, room->name) == 0)  // Se nome da sala ja existe nao cria
			{
				if (user != NULL)               // se lobby estiver sendo criado, usurio pode NULL
				{
					reply(user, "Room already exists. Please try another name.");
				}
				//room mutex end
				pthread_mutex_unlock(&room_m);
				return;
			}

			if (it->next == NULL) //se fim da lista, cria
			{
				it->next = room;
				//room mutex end
				pthread_mutex_unlock(&room_m);
				return;
			}
		}
		rooms = room; //Se ´e primeiro da lista
	}
	pthread_mutex_unlock(&room_m);
}

void create_pvt_room(client_t *pvt_sndr, char *name, char *receiver_nick) //Cria sala pvt
{
	room_t *room = malloc(sizeof(room_t));

	room->name = malloc(strlen(name) + 1);
	strcpy(room->name, name);
	room->max_users = PVT_ROOM_SIZE;
	room->n_users = 0;
	room->pvt_sender = pvt_sndr;
	room->pvt_receiver_nick = malloc(strlen(receiver_nick) + 1);
	strcpy(room->pvt_receiver_nick, receiver_nick);
	room->users = malloc(sizeof(client_t *) * room->max_users);

	int i;
	for (i = 0; i < room->max_users; i++)
	{
		room->users[i] = NULL;
	}
	room->next = NULL;

	pthread_mutex_lock(&room_m);
	{
		room_t *it;
		for (it = rooms; it != NULL; it = it->next)
		{
			if (strcmp(it->name, room->name) == 0)
			{
				reply(pvt_sndr, "Room already exists. Please try another name.");
				//room mutex end
				pthread_mutex_unlock(&room_m);
				return;
			}

			if (it->next == NULL)
			{
				it->next = room;
				//room mutex end
				pthread_mutex_unlock(&room_m);
				return;
			}
		}
		rooms = room;
	}
	pthread_mutex_unlock(&room_m);
}

void join_room(client_t *user, char *name) // Entra na sala
{
	room_t *it;
	room_t *room_to_join = NULL;

	pthread_mutex_lock(&room_m);
	{
		for (it = rooms; it != NULL; it = it->next)
		{
			if (strcmp(it->name, name) == 0)
			{
				if (it->pvt_sender == NULL && it->pvt_receiver_nick == NULL)
				{
					if (it->n_users < it->max_users)        // Se cabe mais cliente
					{
						room_to_join = it;              //gets room
						break;
					}
					else
					{
						reply(user, "Room is full. Please try later.");
						pthread_mutex_unlock(&room_m);
						return;
					}
				}
				else
				{
					reply(user, "Sorry, but that is a private room."); //Se for pvt nao entra
					pthread_mutex_unlock(&room_m);
					return;
				}
			}
		}
	}
	pthread_mutex_unlock(&room_m);

	if (room_to_join == NULL) // Se sala nao existe
	{
		reply(user, "There is no room with that name.");
		return;
	}

	leave_room(user);
	pthread_mutex_lock(&room_m);
	{
		int i;
		for (i = 0; i < room_to_join->max_users; i++)
		{
			if (room_to_join->users[i] == NULL) //Poe usuario na sala
			{
				room_to_join->users[i] = user;
				user->room = room_to_join;
				room_to_join->n_users++;
				break;
			}
		}
	}
	pthread_mutex_unlock(&room_m);

	char buffer[DEFAULT_BUFFER_SIZE];
	sprintf(buffer, "%s%s%s just joined the room.", user->color, user->name, color_red);
	reply_all(user, buffer);
	return;
}

void join_pvt_room(client_t *user_rcvr)
{
	room_t *it;
	client_t *user_sndr;

	leave_room(user_rcvr);
	pthread_mutex_lock(&room_m);
	{
		for (it = rooms; it != NULL; it = it->next) // Compara de sala em sala qual tem o atributo pvt_reciever_nick igual ao do cliente em questao
		{
			if (it->pvt_receiver_nick != NULL)
			{
				if (strcmp(it->pvt_receiver_nick, user_rcvr->name) == 0)
				{
					user_sndr = it->pvt_sender; //gets sender

					it->users[0] = user_rcvr;
					user_rcvr->room = it;
					it->n_users++;

					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&room_m);

	leave_room(user_sndr);
	pthread_mutex_lock(&room_m);
	{
		for (it = rooms; it != NULL; it = it->next)
		{
			if (it->pvt_receiver_nick != NULL)
			{
				if (strcmp(it->pvt_receiver_nick, user_rcvr->name) == 0)
				{
					it->users[1] = user_sndr;
					user_sndr->room = it;
					it->n_users++;

					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&room_m);

	char buffer[DEFAULT_BUFFER_SIZE];
	sprintf(buffer,
		"%s%s%s and %s%s%s just joined a private room.",
		user_sndr->color, user_sndr->name, color_red,
		user_rcvr->color, user_rcvr->name, color_red);
	reply_all(user_rcvr, buffer);
	return;
}

void delete_pvt_room(client_t *user_rcvr) // Deleta sala pvt
{
	room_t *it;
	room_t *room_to_delete;

	pthread_mutex_lock(&room_m);
	{
		for (it = rooms; it != NULL; it = it->next)
		{
			if (it->pvt_receiver_nick != NULL || it->pvt_sender != NULL)
			{
				if (!(strcmp(it->pvt_receiver_nick, user_rcvr->name)) || !(strcmp(it->pvt_sender->name, user_rcvr->name)))      // Compara com sender e reciever
				{
					room_to_delete = it;                                                                                    //gets room
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&room_m);
	delete_room(room_to_delete);
}

void leave_room(client_t *user) // Sai da sala
{
	if (user->room != NULL)
	{
		room_t *room = user->room;
		char buffer[DEFAULT_BUFFER_SIZE];

		sprintf(buffer, "%s%s%s just left the room.", user->color, user->name, color_red);
		reply_all(user, buffer);

		user->room = NULL;
		int i;
		for (i = 0; i < room->max_users; i++)
		{
			if (room->users[i] == user)
			{
				room->users[i] = NULL;
				room->n_users--;
				break;
			}
		}

		if (room->n_users == 0 && strcmp(room->name, MAIN_ROOM) != 0) // Se ´e o ultimo presente na sala, deleta essa sala
		{
			delete_room(room);
		}
	}
}

void delete_room(room_t *room) // Deleta sala
{
	pthread_mutex_lock(&room_m);
	{
		if (rooms == room)
		{
			rooms = room->next;
		}
		else
		{
			room_t *it;
			for (it = rooms; it->next != NULL; it->next)
			{
				if (it->next == room)
				{
					it->next = it->next->next;
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&room_m);
}

void exit_chat(client_t *user) // Sai do chat
{
	leave_room(user);
	pthread_mutex_lock(&client_m);
	{
		if (users == user)
		{
			users = user->next;
		}
		else
		{
			client_t *it;
			for (it = users; it->next != NULL; it->next)
			{
				if (it->next == user)
				{
					it->next = it->next->next;
					break;
				}
			}
		}
		close(user->socket);
	}
	pthread_mutex_unlock(&client_m);
	pthread_exit(0);
}

int was_user_pvt_requested(char *nick) // Ve se o client ja foi requisitado para pvt, se sim, retorna 1
{
	int requested = 0;
	room_t *it;

	pthread_mutex_lock(&room_m);
	{
		for (it = rooms; it != NULL; it = it->next)
		{
			if (it->pvt_receiver_nick != NULL)
			{
				if (strcmp(nick, it->pvt_receiver_nick) == 0)
				{
					requested = 1;
				}
			}
		}
	}
	pthread_mutex_unlock(&room_m);

	return requested;
}

char* get_nick_from_command(client_t *user, const char *str) // Pegar o nick do comando
{
	room_t *room = user->room;
	int i;

	for (i = 0; i < room->max_users; i++)
	{
		if (room->users[i] != NULL && room->users[i] != user)
		{
			char prefix[80] = "";
			strcat(prefix, "/");
			strcat(prefix, room->users[i]->name);
			strcat(prefix, " ");
			int starts_with = strncmp(prefix, str, strlen(prefix));
			if (starts_with == 0)
			{
				return room->users[i]->name;
			}
		}
	}
	return NULL;
}

int is_nick_in_room(client_t *user, const char *nick) //Ve se o nick se encontra, se sim retorna 1
{
	room_t *room = user->room;
	int nick_in_room = 0;
	int i;

	for (i = 0; i < room->max_users; i++)
	{
		if (room->users[i] != NULL && room->users[i] != user)
		{
			char prefix[80];
			strcat(prefix, room->users[i]->name);
			int starts_with = strncmp(prefix, nick, strlen(prefix));
			nick_in_room = (starts_with == 0);
			if (nick_in_room)
			{
				return nick_in_room;
			}
		}
	}
	return nick_in_room;
}

void *client(void *arg)
{
	client_t *user = (client_t*)arg;

	reply(user, "Connected!");
	reply(user, "Type '/help' to list commands.");
	while (1)
	{
		char buffer[DEFAULT_BUFFER_SIZE];
		bzero(buffer, DEFAULT_BUFFER_SIZE);
		int n = read(user->socket, buffer, DEFAULT_BUFFER_SIZE);
		if (n < 0)
		{
			printf("There was an Error reading from socket.\n");
			exit(-1);
		}

		//Comandos do cliente
		if (str_starts_with("/help", buffer))
		{
			help_user(user);
		}
		else if (str_starts_with("/list_rooms", buffer))
		{
			list_rooms(user);
		}
		else if (str_starts_with("/list_users", buffer))
		{
			list_users(user);
		}
		else if (str_starts_with("/leave", buffer))
		{
			if (strcmp(user->room->name, MAIN_ROOM) == 0)
			{
				reply(user, "You are already at the lobby. Use /exit to leave the chat.");
			}
			else
			{
				join_room(user, MAIN_ROOM);
			}
		}
		else if (str_starts_with("/exit", buffer))
		{
			exit_chat(user);
		}
		else if (str_starts_with("/create ", buffer))
		{
			char *name = strchr(buffer, ' ') + 1;
			if (!is_str_empty(name))
			{
				create_room(user, name, DEFAULT_ROOM_SIZE);
			}
			else
			{
				send_message_to_all(user, buffer);
			}
		}
		else if (str_starts_with("/join ", buffer))
		{
			char *name = strchr(buffer, ' ') + 1;
			if (!is_str_empty(name))
			{
				join_room(user, name);
			}
			else
			{
				send_message_to_all(user, buffer);
			}
		}
		else if (str_starts_with("/nick ", buffer))
		{
			char *name = strchr(buffer, ' ') + 1;
			if (!is_str_empty(name))
			{
				user->name = malloc(strlen(name) + 1);
				strcpy(user->name, name);
			}
			else
			{
				send_message_to_all(user, buffer);
			}
		}
		else if (str_starts_with("/pvt ", buffer))
		{
			char *nick = strchr(buffer, ' ') + 1;
			if (!is_str_empty(nick))
			{
				if (is_nick_in_room(user, nick))
				{
					char room_name[80] = "";
					strcat(room_name, PVT_ROOM);
					strcat(room_name, user->name);
					strcat(room_name, "_");
					strcat(room_name, nick);
					create_pvt_room(user, room_name, nick);
					send_message_to_user(user, PVT_REQUEST, nick);
				}
				else
				{
					reply(user, "There is no user with the given nickname in this room.");
				}
			}
			else
			{
				send_message_to_all(user, buffer);
			}
		}
		else if (str_starts_with("/yes", buffer))
		{
			if (was_user_pvt_requested(user->name))
			{
				join_pvt_room(user);
			}
			else
			{
				send_message_to_all(user, buffer);
			}
		}
		else if (str_starts_with("/no", buffer))
		{
			if (was_user_pvt_requested(user->name))
			{
				delete_pvt_room(user);
			}
			else
			{
				send_message_to_all(user, buffer);
			}
		}
		else if (str_starts_with_nick(user, buffer))
		{
			char *nick = get_nick_from_command(user, buffer);
			char *msg = strchr(buffer, ' ') + 1;
			if (nick != NULL && msg != NULL)
			{
				send_message_to_user(user, msg, nick);
			}
			else
			{
				send_message_to_all(user, buffer);
			}
		}
		else
		{
			send_message_to_all(user, buffer);
		}
	}
}
