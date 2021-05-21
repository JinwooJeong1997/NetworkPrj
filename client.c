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
#include <errno.h>

extern int errno;

#define DATA_SOCK 0
#define MSG_SOCK 1

#define MAX_CMD 100

void sendFile(char *name, int socks);
void error_handling(char *message);


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

		error_handling("fopenfail!");
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
	fclose(file);
}

int cmdchk(const char *str, const char *pre)
{
	size_t lenpre = strlen(pre);
	size_t lenstr = strlen(str);
	return (lenstr < lenpre) ? 0 : memcmp(pre, str, lenpre) == 0;
}

void client_process(int sock, char *buffer)
{
	// Prepare
	char *full_command = malloc(strlen(buffer) + 1);
	strcpy(full_command, buffer);
	char *blank = " ";
	char *command = strtok(full_command, blank);
	char *context = strtok(NULL, blank);

	// Process
	if (cmdchk(command, "pull")){
		client_pull(sock,buffer,context);
	}
	else if (cmdchk(command, "push")){
		client_push(sock,buffer,context);
	}
	else if(cmdchk(command,"ls")){

	}
	else
	{
		printf("No such command: %s\n", buffer);
	}

	// Cleanup
	free(full_command);
}

void client_pull(int sock, char *buffer, char *target_file)
{
	FILE *fd = fopen(target_file, "wb");
	if (fd == NULL)
	{
		fprintf(stderr, "can't create file");
		perror("");
		return;
	}
	ssize_t chunk_size;
	long received_size = 0;

	if (send(sock, buffer, strlen(buffer) + 1, 0) == -1)
	{
		fprintf(stderr, "can't send packet");
		perror("");
		fclose(fd);
		return;
	}

	// Retrieve File Size
	char response[1024];
	if (recv(sock, response, sizeof(response), 0) == -1)
	{
		fprintf(stderr, "can't receive packet");
		perror("");
		fclose(fd);
		return;
	}
	long file_size = strtol(response, NULL, 0);

	// Notify server to start transmission
	strcpy(buffer, "ready");
	if (send(sock, buffer, strlen(buffer) + 1, 0) == -1)
	{
		fprintf(stderr, "can't send packet");
		perror("");
		fclose(fd);
		return;
	}

	// Start Receiving
	while (received_size < file_size &&
		   (chunk_size = recv(sock, response, sizeof(response), 0)) > 0)
	{
		if (received_size + chunk_size > file_size)
		{
			fwrite(response, 1, file_size - received_size, fd);
			received_size += file_size - received_size;
		}
		else
		{
			fwrite(response, 1, chunk_size, fd);
			received_size += chunk_size;
		}
	}

	// Clean Up
	printf("%s Downloaded\n", target_file);
	fclose(fd);
}

void client_push(int sock, char *buffer, char *target_file)
{
	// Send Upload Command
	if (send(sock, buffer, strlen(buffer) + 1, 0) == -1)
	{
		fprintf(stderr, "can't send packet");
		perror("");
		return;
	}

	// Initialize File Descriptor
	FILE *fd = fopen(target_file, "rb");
	if (fd == NULL)
	{
		fprintf(stderr, "can't create file ");
		perror("");
		return;
	}
	ssize_t chunk_size;

	// Wait for server to be ready
	char response[1024];
	if (recv(sock, response, sizeof(response), 0) == -1)
	{
		fprintf(stderr, "can't receive packet");
		perror("");
		fclose(fd);
		return;
	}
	if (begin_with(response, "@"))
	{
		printf("Server Error: %s\n", &response[1]);
		fclose(fd);
		return;
	}

	// Notify File Size
	fseek(fd, 0L, SEEK_END);
	sprintf(buffer, "%ld", ftell(fd));
	if (send(sock, buffer, strlen(buffer) + 1, 0) == -1)
	{
		fprintf(stderr, "can't send packet");
		perror("");
		fclose(fd);
		return;
	}
	fseek(fd, 0L, SEEK_SET);

	// Wait for server to be ready
	if (recv(sock, response, sizeof(response), 0) == -1)
	{
		fprintf(stderr, "can't receive packet");
		perror("");
		fclose(fd);
		return;
	}

	// Start Transmission
	while ((chunk_size = fread(buffer, 1, sizeof(buffer), fd)) > 0)
	{
		if (send(sock, buffer, chunk_size, 0) == -1)
		{
			fprintf(stderr, "can't send packet");
			perror("");
			fclose(fd);
			return;
		}
	}

	// Clean Up
	printf("%s Uploaded\n", target_file);
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
	int sock;
	int str_len, len;
	struct sockaddr_in serv_addr;
	char buffer[1024];
	char message[MAX_CMD];
	if (argc != 3)
	{
		printf("Usage : %s <IP> <PORT> \n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		error_handling("socket() error");
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));
	printf("serv_addr port : %d \n", ntohs(serv_addr.sin_port));
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
	{
		error_handling("connect() error!");
	}
	// test
	str_len = read(sock, message, BUFSIZ - 1);

	if (str_len == -1)
	{
		error_handling("read() error!");
	}

	printf("Message from server : %s \n", message);
	char cmd[MAX_CMD];
	//fgets(cmd, MAX_CMD, stdin);
	//sendFile(cmd, sock);
	//printf("file send comp!\n");
	/*while (1)
	{
		memset(message, 0, sizeof(message) * sizeof(char));
		fputs("input msg (q to Quit) : ", stdout);
		fgets(message, BUFSIZ, stdin);
		write(sock, message, strlen(message));
		if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
		{
			break;
		}
		str_len = read(sock, message, BUFSIZ - 1);
		if (str_len == -1)
		{
			error_handling("read() error!");
		}
		message[str_len] = 0;
		printf("Message from server MSG_SOCK: %s \n", message);
	}*/
	client_process(sock,buffer);

	close(sock);
	return 0;
}