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
#include <signal.h>

extern int errno;

#define DATA_SOCK 0
#define MSG_SOCK 1

#define MAX_CMD 100
<<<<<<< HEAD



=======
>>>>>>> test1

void error_handling(char *message);
int client_pull(int sock, char *buffer, char *target_file);
int client_push(int sock, char *buffer, char *target_file);
int client_process(int sock, char *buffer);
int cmdchk(const char *str, const char *pre);
int sendMsg(int sock,char msg[]);

int sendMsg(int sock,char msg[]){
	if((send(sock,msg,strlen(msg)+1,0))==-1){
		fprintf(stderr,"(%d) can't packet\n",sock);
		return errno;
	}
	return 0;
}


int cmdchk(const char *str, const char *pre){
	size_t lenpre = strlen(pre);
	size_t lenstr = strlen(str);
	return (lenstr < lenpre) ? 0 : memcmp(pre, str, lenpre) == 0;
}

<<<<<<< HEAD
//client process
int client_process(int sock, char *buffer){
	// Prepare
=======
//클라이언트 동작이 동작하는 곳
int client_process(int sock, char *buffer){
>>>>>>> test1
	char *full_command = malloc(strlen(buffer) + 1);
	strcpy(full_command, buffer);
	char *blank = " ";
	char *command = strtok(full_command, blank);
	char *context = strtok(NULL, blank);
	int rs = -1;
<<<<<<< HEAD
	printf("INPUT : %s\n", buffer);
	printf("command %s(%d)\ncontent %s(%d) \n",command,strlen(command),context,strlen(context));
	// Process
	if (cmdchk(command, "pull")){
		printf("pull!\n");
		rs = client_pull(sock,buffer,context);
	}
	else if (cmdchk(command, "push")){
		printf("push!\n");
		rs = client_push(sock,buffer,context);
	}
	else if(cmdchk(command,"ls")){
		printf("ls\n");
=======


	// Process
	if (cmdchk(command, "pull")){
		rs = client_pull(sock,buffer,context);
	}
	else if (cmdchk(command, "push")){
		rs = client_push(sock,buffer,context);
	}
	else if(cmdchk(command,"ls"))
	{
		//추가 할곳.
>>>>>>> test1
	}
	else if(cmdchk(command,"vim")){
		rs = client_vim(sock,buffer,context);
	}
	else{
		printf("No such command: %s\n", buffer);
	}

	// Cleanup
	free(full_command);
	return rs;
}

int client_pull(int sock, char *buffer, char *target_file){
	FILE *fd = fopen(target_file, "wb");
<<<<<<< HEAD
	printf("client_pull-fp! \n");
=======
>>>>>>> test1
	if (fd == NULL){
		fprintf(stderr, "can't create file");
		perror("");
		return -1;
	}
	ssize_t chunk_size;
	long received_size = 0;
<<<<<<< HEAD
	printf("client_pull-pull cmd! \n");
=======
>>>>>>> test1
	//send Pull command
	if (send(sock, buffer, strlen(buffer) + 1, 0) == -1){
		fprintf(stderr, "can't send packet");
		perror("");
		unlink(target_file);
		fclose(fd);
		return -1;
	}
<<<<<<< HEAD
	printf("client_pull-pull retrieve! \n");
=======
>>>>>>> test1
	// Retrieve File Size
	char response[1024];
	if (recv(sock, response, sizeof(response), 0) == -1){
		fprintf(stderr, "can't receive packet");
		perror("");
		unlink(target_file);
		fclose(fd);
		return -1;
	}
	if(cmdchk(response,"@")){
		printf("Server Error: %s\n",&response[1]);
		unlink(target_file);
		fclose(fd);
		return -1;
	}
	long file_size = strtol(response, NULL, 0);
<<<<<<< HEAD
	printf("client_pull-pull notify! \n");
=======
>>>>>>> test1
	// Notify server to start transmission
	strcpy(buffer, "ready");
	if (send(sock, buffer, strlen(buffer) + 1, 0) == -1){
		fprintf(stderr, "can't send packet");
		perror("");
		unlink(target_file);
		fclose(fd);
		return-1;
	}
<<<<<<< HEAD
	printf("client_pull-receving! \n");
=======

>>>>>>> test1
	// Start Receiving
	while (received_size < file_size &&(chunk_size = recv(sock, response, sizeof(response), 0)) > 0){
		if (received_size + chunk_size > file_size){
			fwrite(response, 1, file_size - received_size, fd);
			received_size += file_size - received_size;
		}
		else{
			fwrite(response, 1, chunk_size, fd);
			received_size += chunk_size;
		}
	}
<<<<<<< HEAD
	printf("client_pull-cleanup!! \n");
=======

>>>>>>> test1
	// Clean Up
	printf("%s Downloaded\n", target_file);
	fclose(fd);
	return 0;
}
int client_push(int sock, char *buffer, char *target_file){
<<<<<<< HEAD
	printf("push in\n");
=======
>>>>>>> test1
	// Send Upload Command
	if (send(sock, buffer, strlen(buffer) + 1, 0) == -1)
	{
		fprintf(stderr, "can't send packet");
		perror("client_push_send():");
		return -1;
	}
	printf("push init\n");	
	// Initialize File Descriptor
	FILE *fd = fopen(target_file, "rb");
	if (fd == NULL)
	{
		fprintf(stderr, "can't create file ");
		perror("client_push_fopen():");
		return -1;
	}
	ssize_t chunk_size;
<<<<<<< HEAD
	printf("push wait\n");
	// Wait for server to be ready
=======

	//  서버 응답 체크
>>>>>>> test1
	char response[1024];
	if (recv(sock, response, sizeof(response), 0) == -1)
	{
		fprintf(stderr, "can't receive packet");
		perror("client_push_recv");
		fclose(fd);
		return -1;
	}
<<<<<<< HEAD
=======
	//에러 발생 확인
>>>>>>> test1
	if(cmdchk(response,"@")){
		printf("Server Error: %s\n",&response[1]);
		fclose(fd);
		return -1;
	}
<<<<<<< HEAD
	printf("push notifiy\n");
=======
>>>>>>> test1
	// Notify File Size
	fseek(fd, 0L, SEEK_END);
	sprintf(buffer, "%ld", ftell(fd));
	if (send(sock, buffer, strlen(buffer) + 1, 0) == -1)
	{
		fprintf(stderr, "can't send packet");
		perror("client_push_NotifyFileSize ");
		fclose(fd);
		return -1;
	}
	fseek(fd, 0L, SEEK_SET);
<<<<<<< HEAD
	printf("push wait ready\n");
=======
>>>>>>> test1
	// Wait for server to be ready
	if (recv(sock, response, sizeof(response), 0) == -1)
	{
		fprintf(stderr, "can't receive packet");
		perror("client_push_wait_recv");
		fclose(fd);
		return -1;
	}
<<<<<<< HEAD
	printf("push trans!\n");
=======

>>>>>>> test1
	// Start Transmission
	while ((chunk_size = fread(buffer, 1, sizeof(buffer), fd)) > 0)
	{
		if (send(sock, buffer, chunk_size, 0) == -1)
		{
			fprintf(stderr, "can't send packet");
			perror("client_push_Transmission");
			fclose(fd);
			return -1;
		}
	}

	// Clean Up
	printf("%s Uploaded\n", target_file);
	fclose(fd);
	return 0;
}
int client_vim(int sock,char *buffer,char *target_file){
	//check file exist in server
<<<<<<< HEAD
	printf("client_vim entered!\n");
=======
>>>>>>> test1
	pid_t pid = 0;
	int status;
	char *cmd = malloc(sizeof(char) * 1024);

<<<<<<< HEAD
=======

>>>>>>> test1
	strcpy(buffer,"pull ");
	strcat(buffer,target_file);
	strcat(buffer,"\0");
	printf("VIM BEFORE :%s\n",buffer);
<<<<<<< HEAD
	client_process(sock,buffer);
	sleep(1);
	//vi!
=======

	if(client_process(sock,buffer)==-1){
		printf("%s not exist at server \n",target_file);
	}

	//파일 전송대기를 위한 sleep()함수
	sleep(1);

>>>>>>> test1
	strcpy(cmd,"vi ");
	strcat(cmd,target_file);
	system(cmd);
	
<<<<<<< HEAD
	printf("exiting vim\n");
	strcpy(buffer,"push ");
	strcat(buffer,target_file);
	strcat(buffer,"\0");
	printf("VIM AFTER :%s\n",buffer);
=======
	strcpy(buffer,"push ");
	strcat(buffer,target_file);
	strcat(buffer,"\0");
>>>>>>> test1
	client_process(sock,buffer);

	free(cmd);
	unlink(target_file);
	printf("vim opened!\n");
	return 0;
}

void error_handling(char *message){
	perror(message);
<<<<<<< HEAD
	//fputs(message, stderr);
	//fputc('\n', stderr);
=======
>>>>>>> test1
	exit(1);
}

int main(int argc, char *argv[]){
	int sock;
	int str_len, len;
	struct sockaddr_in serv_addr;
	char buffer[1024];
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
<<<<<<< HEAD
=======

>>>>>>> test1
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));
	printf("serv_addr port : %d \n", ntohs(serv_addr.sin_port));
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
	{
		perror("connect() error!");
		close(sock);
		exit(errno);
	}
<<<<<<< HEAD
=======


>>>>>>> test1
	while(1){
		//read
		printf(">>");
		fgets(buffer,1024,stdin);
		printf("%s \n",buffer);
		if(!strcmp(buffer,"\n")){
			continue;
		}
		if((strlen(buffer)>0)&&(buffer[strlen(buffer)-1])=='\n'){
			buffer[strlen(buffer)-1] = '\0';
		}
		if(strcmp(buffer,"exit")== 0){
			break;
		}
		printf("Message : %s \n", buffer);
		client_process(sock,buffer);
	}
<<<<<<< HEAD
	
=======
>>>>>>> test1
	close(sock);
	return 0;
}