//서버
//데이터 송/수신용 소켓 _sock
//메세지 송/수신용 소켓 _msg_cock

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>

#define DATA_SOCK 0
#define MSG_SOCK 1

#define BACKLOG 5
#define MAX_CMD 100
char message[MAX_CMD];

void error_handling(char *message);

void *handle_clnt_msg(void *arg){};
void *handle_clnt_dat(void *arg){};

int main(int argc, char *argv[])
{
	int serv_sock[2];
	int clnt_sock[2];
	int str_len;
	char buf[256];
	struct sockaddr_in serv_addr[2];
	struct sockaddr_in clnt_addr[2];
	socklen_t clnt_addr_size[2];

	strcpy(message, "hello world!");

	if (argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	for (int i = 0; i < 2; i++)
	{
		//init socks
		serv_sock[i] = socket(PF_INET, SOCK_STREAM, 0);
		if (serv_sock[i] == -1)
		{
			error_handling("socket() error");
		}
		printf("socket[%d] socket() succeed \n ", i);
		memset(&serv_addr[i], 0, sizeof(serv_addr[i]));
		serv_addr[i].sin_family = AF_INET;
		serv_addr[i].sin_addr.s_addr = htonl(INADDR_ANY);
		serv_addr[i].sin_port = htons(atoi(argv[1]) + i);
		printf("serv_addr[%d] port : %d \n", i, ntohs(serv_addr[i].sin_port));


		if (bind(serv_sock[i], (struct sockaddr *)&serv_addr[i], sizeof(serv_addr[i])) == -1)
		{
			error_handling("bind() error");
		}
		printf("socket[%d] bind() succeed \n ",i);
	}

	if (listen(serv_sock[0], 5) == -1)
		{
			error_handling("listen() error");
		}
		printf("socket[1] listen() succeed \n ");

		clnt_addr_size[0] = sizeof(clnt_addr[i]);
		clnt_sock[0] = accept(serv_sock[i], (struct sockaddr *)&clnt_addr[i], &clnt_addr_size[i]);
		if (clnt_sock[0] == -1)
		{
			printf("%d \n ", 0);
			error_handling("accept() error");
		}
	printf("socket0 accept() succeed \n ");

	if (listen(serv_sock[1], 5) == -1)
		{
			error_handling("listen() error");
		}
		printf("socket[%d] listen() succeed \n ", i);

		clnt_addr_size[1] = sizeof(clnt_addr[i]);
		clnt_sock[1] = accept(serv_sock[i], (struct sockaddr *)&clnt_addr[i], &clnt_addr_size[i]);
		if (clnt_sock[1] == -1)
		{
			printf("%d \n ", 1);
			error_handling("accept() error");
		}
	printf("socket0 accept() succeed \n ");
	/*for (int i = 0; i < 2; i++)
	{
		if (listen(serv_sock[i], 5) == -1)
		{
			error_handling("listen() error");
		}
		printf("socket[%d] listen() succeed \n ", i);

		clnt_addr_size[i] = sizeof(clnt_addr[i]);
		clnt_sock[i] = accept(serv_sock[i], (struct sockaddr *)&clnt_addr[i], &clnt_addr_size[i]);
		if (clnt_sock[i] == -1)
		{
			printf("%d \n ", i);
			error_handling("accept() error");
		}
		printf("socket[%d] accept() succeed \n ", i);
	}*/

	write(clnt_sock[MSG_SOCK], message, strlen(message) - 1);
	int nbyte = 256;
	size_t filesize = 0, bufsize = 0;
	FILE *file = NULL;
	file = fopen("test.txt", "wb");
	bufsize = 256;
	while (nbyte != 0)
	{
		nbyte = recv(clnt_sock[DATA_SOCK], buf, bufsize, 0);
		fwrite(buf, sizeof(char), nbyte, file);
	}
	fclose(file);
	printf("file update complete!\n");
	while (1)
	{
		str_len = read(clnt_sock[MSG_SOCK], message, BUFSIZ - 1);
		if (str_len == -1)
		{
			error_handling("read() error!");
		}
		if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
		{
			close(clnt_sock[MSG_SOCK]);
			break;
		}
		write(clnt_sock[MSG_SOCK], message, strlen(message));
		message[str_len] = 0;
		printf("Message from client MSG_SOCK: %s \n", message);
	}
	for (int i = 0; i < 2; i++)
	{
		close(clnt_sock[i]);
		close(serv_sock[i]);
	}
	return 0;
}

void error_handling(char *message)
{
	perror(message);
	//fputs(message, stderr);
	//fputc('\n', stderr);
	exit(1);
}
