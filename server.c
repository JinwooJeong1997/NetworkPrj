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


void *server_thread(void *sock){
	int buffer_size = 1024;
	char *buffer = malloc(sizeof(char) * buffer_size);
	while (1){
		//명령어 입력확인
		if (recv((int)sock, buffer, buffer_size, 0) < 1){
			fprintf(stderr, "%d Terminated", (int)sock);
			perror("");
			close((int)sock);
			break;
		}
		server_process((int)sock, buffer);
	}
}

int cmdchk(const char *str, const char *pre){
	size_t lenpre = strlen(pre);
	size_t lenstr = strlen(str);
	return (lenstr < lenpre) ? 0 : memcmp(pre, str, lenpre) == 0;
}

void server_process(int sock, char *command){
	char *blank = " ";
	char *cmd = strtok(command, blank);
	char *context = strtok(NULL, blank);
	char *response = malloc(sizeof(char) * 1024);
	printf("received msg from (%d) : %s",sock,command);
	if (cmdchk(cmd, "pull")){
		server_pull(sock,context);
	}
	else if (cmdchk(cmd, "push")){
		server_push(sock,context);
	}
	else if (cmdchk(cmd, "list")){
		//show lists
	}
	free(response);
}

void server_pull(int sock, char *target_file){
	FILE *fd;
	if ((fd = fopen(target_file, "rb")) == NULL){
		perror("");
		return;
	}

	char buffer[1024];
	ssize_t chunk_size;

	fseek(fd, 0L, SEEK_END);
	sprintf(buffer, "%ld", ftell(fd));
	ssize_t byte_sent = send(sock, buffer, strlen(buffer) + 1, 0);
	if (byte_sent == -1){
		fprintf(stderr, "(%d) : can't send packet", sock);
		perror("");
		fclose(fd);
		return;
	}
	fseek(fd, 0L, SEEK_SET);

	// Wait for client to be ready
	ssize_t byte_received = recv(sock, buffer, sizeof(buffer), 0);
	if (byte_received == -1){
		fprintf(stderr, "(%d) can't receive packet", sock);
		perror("");
		fclose(fd);
		return;
	}

	// Start Transmission
	while ((chunk_size = fread(buffer, 1, sizeof(buffer), fd)) > 0){
		ssize_t byte_sent = send(sock, buffer, chunk_size, 0);
		if (byte_sent == -1){
			fprintf(stderr, "(%d) can't send packet", sock);
			perror("");
			fclose(fd);
			return;
		}
	}
	printf("(%d) Transmited: %s\n", sock, target_file);
	fclose(fd);
}

void server_push(int sock, char *target_file)
{
	// Initialize File Descriptor
	FILE *fd;
	if ((fd = fopen(target_file, "wb")) == NULL)
	{
		fprintf(stderr, "(%d) Can't open %s", sock, target_file);
		perror("");
		return;
	}

	// Retrieve File Size
	char buffer[1024];
	strcpy(buffer, "size?");
	ssize_t byte_sent = send(sock, buffer, strlen(buffer) + 1, 0);
	if (byte_sent == -1)
	{
		fprintf(stderr, "(%d) can't send packet", sock);
		perror("");
		fclose(fd);
		return;
	}
	ssize_t byte_received = recv(sock, buffer, sizeof(buffer), 0);
	if (byte_received == -1)
	{
		fprintf(stderr, "(%d) can't receive packet", sock);
		perror("");
		fclose(fd);
		return;
	}
	long file_size = strtol(buffer, NULL, 0); //strol => coveert string to long int

	// Notify client to start transmission
	strcpy(buffer, "ready");
	byte_sent = send(sock, buffer, strlen(buffer) + 1, 0);
	if (byte_sent == -1)
	{
		fprintf(stderr, "(%d) can't send packet", sock);
		perror("");
		fclose(fd);
		return;
	}

	// Start Receiving
	ssize_t chunk_size;
	long received_size = 0;
	while (received_size < file_size &&
		   (chunk_size = recv(sock, buffer, sizeof(buffer), 0)) > 0)
	{
		if (chunk_size == -1)
		{
			fprintf(stderr, "(%d) can't receive packet", sock);
			perror("");
			fclose(fd);
			return;
		}
		if (received_size + chunk_size > file_size)
		{
			fwrite(buffer, 1, file_size - received_size, fd);
			received_size += file_size - received_size;
		}
		else
		{
			fwrite(buffer, 1, chunk_size, fd);
			received_size += chunk_size;
		}
	}
	fprintf(stderr, "(%d) Saved: %s\n", sock, target_file);
	fclose(fd);
}

void error_handling(char *message)
{
	perror(message);
	//fputs(message, stderr);
	//fputc('\n', stderr);
	exit(1);
}

int main(int argc, char *argv[])
{
	int serv_sock;
	int clnt_sock;
	int str_len;
	char buffer[1024];
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
	//bind
	if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
	{
		error_handling("bind() error");
	}
	//listen
	if (listen(serv_sock, 5) == -1)
	{
		error_handling("listen() error");
		close(clnt_sock);
		exit(errno);
	}
	clnt_addr_size = sizeof(clnt_addr);

	
	while(1){
		//accept
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
		if (clnt_sock == -1){
			error_handling("accept() error");
			continue;
		}
		printf("(%d) Accepted \n",clnt_sock);
		pthread_t thread;
		if (pthread_create(&thread, NULL, server_thread,clnt_sock)){
			printf("(%d) Create thread error\n",clnt_sock);
		}
	}
	close(clnt_sock);
	close(serv_sock);
	return 0;
}