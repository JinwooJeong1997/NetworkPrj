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
int sock;
#define DATA_SOCK 0
#define MSG_SOCK 1

#define MAX_CMD 100
 

void handle_sigint() {
  close(sock);
  printf("클라이언트 종료\n");
  exit(1);
}

void error_handling(char *message);
int client_pull(int sock, char *buffer, char *target_file);
int client_push(int sock, char *buffer, char *target_file);
int client_ls(int sock,char *buffer);
int client_rm(int sock,char *buffer, char *traget_file);
int client_process(int sock, char *buffer);
int cmdchk(const char *str, const char *pre);
int sendMsg(int sock,char msg[]);

int sendMsg(int sock,char msg[]){
	printf("send() from sendMsg(%s)\n",msg);
	if((send(sock,msg,strlen(msg)+1,0))==-1){
		fprintf(stderr,"(%d) can't packet\n",sock);
		return errno;
	}
	return 0;
}

//명령어 체크
int cmdchk(const char *str, const char *pre){
	size_t lenpre = strlen(pre);
	size_t lenstr = strlen(str);
	return (lenstr < lenpre) ? 0 : memcmp(pre, str, lenpre) == 0;
}

//클라이언트 동작이 동작하는 곳
int client_process(int sock, char *buffer){
	char *full_command = malloc(strlen(buffer) + 1);
	strcpy(full_command, buffer);
	char *blank = " ";
	char *command = strtok(full_command, blank);
	char *context = strtok(NULL, blank);
	int rs = -1;

	// Process
	if (cmdchk(command, "pull")){
		rs = client_pull(sock,buffer,context);
	}
	else if(cmdchk(command, "push")){
		rs = client_push(sock,buffer,context);
	}
	else if(cmdchk(command,"ls")){
		rs = client_ls(sock,buffer);
	}
	else if(cmdchk(command,"vim")){
		rs = client_vim(sock,buffer,context);
	}
	else if(cmdchk(command,"rm")){
		rs = client_rm(sock,buffer,context);
	}
	else if(cmdchk(command,"help")){
		printf("pull [파일이름]	: 서버에서 파일을 다운로드 합니다.\n");
		printf("	 	- 반드시 수정후 push를 해야됩니다.\n");
		printf("push [파일이름] : 서버로 파일을 업로드 합니다.\n");
		printf("		- 누군가 사용중인 경우 업로드가 안됩니다.\n");
		printf("ls : 서버내 존재하는 파일을 출력합니다.\n");
		printf("vim [파일이름] : 서버에서 파일을 가져와 수정합니다.\n");
		printf("		- 누군가 사용중인 경우 수정이 불가능합니다.\n");
		printf("		- 서버내 없다면 새로 생성됩니다.\n");
		printf("rm [파일이름] : 서버내 존재하는 파일을 삭제합니다.\n");
		printf("		- 누군가 사용중인 경우 삭제가 불가능합니다.\n");
		printf("exit : 프로그램을 종료합니다\n");
	}
	else{
		printf("해당 명령어는 존재하지 않습니다.: %s\n", buffer);
		printf("help 명령어를 통해 가능한 명령어를 찾아보십시오.\n");
	}

	// Cleanup
	free(full_command);
	return rs;
}

//서버에서 다운로드
int client_pull(int sock, char *buffer, char *target_file){
	FILE *fd = fopen(target_file, "wb");
	if (fd == NULL){
		fprintf(stderr, "can't create file\n");
		perror("");
		return -1;
	}
	ssize_t chunk_size;
	long received_size = 0;
	//send Pull command
	if (send(sock, buffer, strlen(buffer) + 1, 0) == -1){
		fprintf(stderr, "can't send packet\n");
		perror("");
		unlink(target_file);
		fclose(fd);
		return -1;
	}

	// Retrieve File Size
	char response[1024];
	if (recv(sock, response, sizeof(response), 0) == -1){
		fprintf(stderr, "can't receive packet\n");
		perror("");
		unlink(target_file);
		fclose(fd);
		return -1;
	}
	if(cmdchk(response,"@")){
		printf("서버에러 발생 : %s\n",&response[1]);
		unlink(target_file);
		fclose(fd);
		return -1;
	}
	if(cmdchk(response,"!")){
		printf("%s 은(는) 다른 누군가 사용중입니다.\n",target_file);
		unlink(target_file);
		fclose(fd);
		return -2;
	}
	long file_size = strtol(response, NULL, 0);
	// Notify server to start transmission
	strcpy(buffer, "ready");
	if (send(sock, buffer, strlen(buffer) + 1, 0) == -1){
		fprintf(stderr, "can't send packet\n");
		perror("");
		unlink(target_file);
		fclose(fd);
		return-1;
	}
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
	printf("%s 을(를) 다운로드하였습니다.\n", target_file);
	fclose(fd);
	return 0;
}

int client_push(int sock, char *buffer, char *target_file){
	// push 명령 서버에 전달
	if (send(sock, buffer, strlen(buffer) + 1, 0) == -1)
	{
		fprintf(stderr, "can't send packet");
		perror("client_push_send():");
		return -1;
	}
	//  FD 초기화
	FILE *fd = fopen(target_file, "rb");
	if (fd == NULL)
	{
		fprintf(stderr, "파일을 생성할 수 없습니다. \n");
		perror("client_push_fopen():");
		return -1;
	}
	ssize_t chunk_size;

	//  서버 응답 체크
	char response[1024];
	if (recv(sock, response, sizeof(response), 0) == -1)
	{
		fprintf(stderr, "패킷을 수신할 수 없습니다.\n");
		perror("client_push_recv");
		fclose(fd);
		return -1;
	}
	//에러 발생 확인
	if(cmdchk(response,"@")){
		printf("서버 에러: %s\n",&response[1]);
		fclose(fd);
		return -1;
	}
	

	// Notify File Size
	fseek(fd, 0L, SEEK_END);
	sprintf(buffer, "%ld", ftell(fd));
	if (send(sock, buffer, strlen(buffer) + 1, 0) == -1)
	{
		fprintf(stderr, "패킷을 수신할 수 없습니다.\n");
		perror("client_push_NotifyFileSize ");
		fclose(fd);
		return -1;
	}
	fseek(fd, 0L, SEEK_SET);
	// Wait for server to be ready
	if (recv(sock, response, sizeof(response), 0) == -1)
	{
		fprintf(stderr, "패킷을 수신할 수 없습니다.\n");
		perror("client_push_wait_recv");
		fclose(fd);
		return -1;
	}
	// Start Transmission
	while ((chunk_size = fread(buffer, 1, sizeof(buffer), fd)) > 0)
	{
		if (send(sock, buffer, chunk_size, 0) == -1)
		{
			fprintf(stderr, "패킷 송신에 오류가 발생했습니다.\n");
			perror("client_push_Transmission");
			fclose(fd);
			return -1;
		}
	}

	// 업로드 완료
	printf("%s 을(를) 업로드 하였습니다.\n", target_file);
	fclose(fd);
	return 0;
}

int client_rm(int sock, char *buffer,char *target_file){
	if (send(sock, buffer, strlen(buffer) + 1, 0) == -1)
	{
		fprintf(stderr, "패킷 송신에 오류가 발생했습니다.\n");
		perror("client_push_send():");
		return -1;
	}

	char response[1024];
	if(recv(sock,response,sizeof(response),0) == -1){
		fprintf(stderr,"서버가 준비되지 않았습니다.\n");
		return -1;
	}

	//메시지 확인
	if(cmdchk(response,"@")){	//file not exist;
		printf("%s 삭제 실패\n",target_file);
		fprintf(stderr,"삭제에 실패하였습니다.n");
		return -1;
	}else if ( cmdchk(response,"!")){ // file is locked;
		printf("%s 삭제 실패\n",target_file);
		fprintf(stderr,"파일이 사용중입니다\n");
		return -2;

	}else if(cmdchk(response,"#")){	// success!
		printf("%s 삭제 완료\n",target_file);
		return 0;
	}
	return -1;
}

//vim 기능
int client_vim(int sock,char *buffer,char *target_file){
	int status;
	char *cmd = malloc(sizeof(char) * 1024);


	//pull 명령어 실행
	memset(buffer,'\0',sizeof(buffer));
	strcpy(buffer,"pull ");
	strcat(buffer,target_file);
	strcat(buffer,"\0");
	status = client_pull(sock,buffer,target_file);

	if(status == -1){
		printf("%s 은(는) 존재하지 않습니다. 새로 생성합니다.\n",target_file);
	}else if(status == -2){	// lock 상태
		printf("%s 은(는) 현재 사용중 입니다. 나중에 편집해주세요.\n",target_file);
		return -1;
	}


	//파일 전송대기를 위한 sleep()함수
	sleep(1);

	strcpy(cmd,"vi ");
	strcat(cmd,target_file);
	system(cmd);
	
	//errno 를 통해 에러 발생여부 확인
	if(errno != 0){
		printf("편집중 오류 발생\n");
		printf("다시 시도해주세요!\n");
		errno = 0;
		return -1;
	}
	//입력 스트림 초기화
	__fpurge(stdin);
	
	//push 명령어 실행
	memset(buffer,'\0',sizeof(buffer));
	strcpy(buffer,"push ");
	strcat(buffer,target_file);
	strcat(buffer,"\0");
	printf(" after vim buffer (%s) \n",buffer);
	client_push(sock,buffer,target_file);
	free(cmd);
	unlink(target_file);
	return 0;
}

int client_ls(int sock,char *buffer){
	// 서버에 명령어 보냄
	if (send(sock, buffer, sizeof(buffer), 0) < 0)
	{
		fprintf(stderr, "패킷으로 송신할수 없습니다.\n");
		perror("client_ls():");
		return -1;
	}

	//서버 준비 확인
	char response[1024];
	if( recv(sock,response,sizeof(response),0) == -1){
		fprintf(stderr,"서버가 준비되지 않았습니다.\n");
		return -1;
	}
	//서버준비확인 -> 오류 체크
	if(cmdchk(response,"@")){
		fprintf(stderr,"파일이 준비되지 않았습니다.\n");
		return -1;
	}
	int stat = 0;
	while((stat=recv(sock,response,sizeof(response),0)) >  0){
		if(cmdchk(response,"#")){
			printf("파일의 끝\n");
			break;
		}
		response[strlen(response)+1] = "\n";
		printf("=%s=\n",response);
		memset(response,'\0',sizeof(buffer));
	}
	return 0;
}
//에러 메시지 보내는 함수(프로그램 종료)
void error_handling(char *message){
	perror(message);
	exit(1);
}

int main(int argc, char *argv[]){
	int sock;
	int str_len, len;
	struct sockaddr_in serv_addr;
	char buffer[1024];
	if (argc != 3){
		printf("사용방법 : %s <IP> <PORT> \n", argv[0]);
		exit(1);
	}
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1){
		error_handling("socket() error");
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));
	printf("서버 접속 : %d \n", ntohs(serv_addr.sin_port));
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
		perror("connect() error!");
		close(sock);
		exit(errno);
	}
	signal(SIGINT, handle_sigint);
	while(1){
		//read
		printf(">>");
		fgets(buffer,1024,stdin);
		if(!strcmp(buffer,"\n")){
			continue;
		}
		if((strlen(buffer)>0)&&(buffer[strlen(buffer)-1])=='\n'){
			buffer[strlen(buffer)-1] = '\0';
		}
		if(strcmp(buffer,"exit")== 0){
			break;
		}
		client_process(sock,buffer);
	}
	close(sock);
	return 0;
}
