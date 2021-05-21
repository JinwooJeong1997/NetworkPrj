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
	int serv_sock;
	int clnt_sock;
	int str_len;
	char buf[256];
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;

	strcpy(message, "hello world!");

	if (argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	//init socks
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (serv_sock == -1)
	{
		error_handling("socket() error");
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));
	printf("serv_addr port : %d \n", ntohs(serv_addr.sin_port));

	if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
	{
		error_handling("bind() error");
	}
	if (listen(serv_sock, 5) == -1)
	{
		error_handling("listen() error");
	}

	clnt_addr_size = sizeof(clnt_addr);
	clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
	if (clnt_sock == -1)
	{
		error_handling("accept() error");
	}

	write(clnt_sock, message, strlen(message) - 1);
	int nbyte = 256;
	/*
	size_t filesize = 0, bufsize = 0;
	FILE *file = NULL;
	file = fopen("test.txt", "wb");
	bufsize = 256;

	while (nbyte != 0)
	{
		nbyte = recv(clnt_sock, buf, bufsize, 0);
		printf("%d byte \n", nbyte);
		fwrite(buf, sizeof(char), nbyte, file);
	}
	fclose(file);
	printf("file update complete!\n");
	*/
	while (1)
	{
		str_len = read(clnt_sock, message, BUFSIZ - 1);
		if (str_len == -1)
		{
			error_handling("read() error!");
		}
		if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
		{
			break;
		}
		printf("Message from client MSG_SOCK: %s \n", message);
		write(clnt_sock, message, strlen(message));
		memset(message,0,sizeof(message)*sizeof(char));
	}

	close(clnt_sock);
	close(serv_sock);
	return 0;
}

void error_handling(char *message)
{
	perror(message);
	//fputs(message, stderr);
	//fputc('\n', stderr);
	exit(1);
}
