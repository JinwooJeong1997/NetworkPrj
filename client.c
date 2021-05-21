/* client.c */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define DATA_SOCK 0
#define MSG_SOCK 1

#define MAX_CMD 100

void sendFile(char *name, int socks);
void error_handling(char *message);
int main(int argc, char *argv[])
{
	int sock[2];
	//int clnt_sock[2];
	int str_len, len;
	struct sockaddr_in serv_addr[2];
	char message[MAX_CMD];
	if (argc != 3)
	{
		printf("Usage : %s <IP> <PORT> \n", argv[0]);
		exit(1);
	}

	for (int i = 0; i < 2; i++)
	{
		sock[i] = socket(PF_INET, SOCK_STREAM, 0);
		if (sock[i] == -1)
		{
			error_handling("socket() error");
		}
		memset(&serv_addr[i], 0, sizeof(serv_addr[i]));
		serv_addr[i].sin_family = AF_INET;
		serv_addr[i].sin_addr.s_addr = inet_addr(argv[1]);
		serv_addr[i].sin_port = htons(atoi(argv[2])+i);
		printf("serv_addr port : %d \n", serv_addr[i].sin_port);
	}

	for(int i = 0; i < 2; i++){
		if (connect(sock[i], (struct sockaddr *)&serv_addr[i], sizeof(serv_addr[i])) == -1)
		{
			error_handling("connect() error!");
		}
	}
	// test
		str_len = read(sock[MSG_SOCK], message, BUFSIZ - 1);

		if (str_len == -1)
		{
			error_handling("read() error!");
		}

		printf("Message from server MSG_SOCK: %s \n", message);
		char cmd[MAX_CMD];
		fgets(cmd, MAX_CMD, stdin);
		sendFile(cmd, sock[DATA_SOCK]);
		printf("file send comp!\n");


	while (!strcmp(message, "q\n"))
	{
		fputs("input msg (q to Quit) : ", stdout);
		fgets(message, BUFSIZ, stdin);
		write(sock[MSG_SOCK], message, strlen(message));
		str_len = read(sock[MSG_SOCK], message, BUFSIZ - 1);
		if (str_len == -1)
		{
			error_handling("read() error!");
		}
		printf("Message from server MSG_SOCK: %s \n", message);
	}

	for (int i = 0; i < 2; i++)
	{
		close(sock[i]);
	}
	return 0;
}

void sendFile(char *name, int socks)
{
	size_t fsize, nsize = 0;
	size_t fsize2;
	char *fname = (char *)malloc(strlen(name));
	strncpy(fname, name, sizeof(name));
	char buf[BUFSIZ];
	FILE *file = NULL;
	/* 전송할 파일 이름을 작성합니다 */
	file = fopen(fname, "rb");
	if (file == NULL)
	{
		printf("%s \n", fname);
		fprintf(stderr, "fopenfail!");
		exit(1);
	}
	printf("file open!\n");
	/* 파일 크기 계산 */
	// move file pointer to end
	fseek(file, 0, SEEK_END);
	// calculate file size
	fsize = ftell(file);
	// move file pointer to first
	fseek(file, 0, SEEK_SET);

	// send file contents
	while (nsize != fsize)
	{
		// read from file to buf
		// 1byte * 256 count = 256byte => buf[256];
		int fpsize = fread(buf, 1, 256, file);
		nsize += fpsize;
		printf("%d / %d \n", fpsize, nsize);
		send(socks, buf, fpsize, 0);
	}
	free(fname);
	write(socks, "END", strlen("END"));
	fclose(file);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
