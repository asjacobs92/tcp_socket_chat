#include "../include/client.h"

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("To use this program, you should type the following command line:\n");
		printf("    ./client <host_addr>\n");

		return -1;
	}

	host = gethostbyname(argv[1]);
	if (host == NULL)
	{
		fprintf(stderr, "Host does not exist.\n");
		exit(0);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("There was an Error opening socket.\n");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr = *((struct in_addr*)host->h_addr);
	bzero(&(serv_addr.sin_zero), 8);

	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("There was an Error connecting.\n");
		exit(-1);
	}

	char buffer[DEFAULT_BUFFER_SIZE];
	bzero(buffer, DEFAULT_BUFFER_SIZE);
	printf("Type in your nickname: ");
	fgets(buffer, DEFAULT_NICK_SIZE, stdin);

	char *pos;
	if ((pos = strchr(buffer, '\n')) != NULL)
	{
		*pos = '\0';
	}

	write(sockfd, buffer, DEFAULT_BUFFER_SIZE);
	pthread_create(&reader, NULL, server, NULL);
	while (1)
	{
		bzero(buffer, DEFAULT_BUFFER_SIZE);
		__fpurge(stdin);
		fgets(buffer, DEFAULT_BUFFER_SIZE, stdin);

		pthread_mutex_lock(&client_m);
		{
			if ((pos = strchr(buffer, '\n')) != NULL)
			{
				*pos = '\0';
			}

			write(sockfd, buffer, DEFAULT_BUFFER_SIZE);
			if (str_starts_with("/exit", buffer))
			{
				close(sockfd);
				puts("Leaving chat...");
				pthread_mutex_unlock(&client_m);
				exit(0);
			}
		}
		pthread_mutex_unlock(&client_m);
	}
	return 0;
}

void *server(void *arg)
{
	char buffer[DEFAULT_BUFFER_SIZE];

	while (1)
	{
		bzero(buffer, DEFAULT_BUFFER_SIZE);
		int n = read(sockfd, buffer, DEFAULT_BUFFER_SIZE);
		pthread_mutex_lock(&client_m);
		{
			if (n < 0)
			{
				printf("There was an Error reading from socket\n");
				exit(-1);
			}
			printf("%s\n", buffer);
		}
		pthread_mutex_unlock(&client_m);
	}
}

int str_starts_with(const char *prefix, const char *str)
{
	int rc;

	rc = strncmp(prefix, str, strlen(prefix));
	return rc == 0;
}
