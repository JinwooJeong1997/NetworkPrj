//서버

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#define DATA_SOCK 0
#define MSG_SOCK 1

#define BACKLOG 5
#define MAX_CMD 100
#define MAX_CLNT 20

int serv_sock;
void* clnt_sock;

pthread_mutex_t mutex;

//단일 연결리스트
typedef struct flist{
	char name[1024];
	int lock;
	struct flist* next;
}fList;

fList * fileHead;


void *server_thread(void *sock);
int cmdchk(const char *str, const char *pre);
int request_pull(int sock, char *target_file);
int request_push(int sock, char *target_file);
int request_ls(int sock);
int request_rm(int sock, char * target_file);

fList* findList(char * target_file){
	//check head is null.
	if(fileHead->next == NULL){
		return NULL;
	}
	fList * ch = fileHead->next;
	
	while(ch != NULL){
		if(strcmp(target_file,ch->name)==0){
			return ch;
		}
		else{
			ch = ch->next;
		}
	}
	return NULL;
}

void printList(){
	fList * itr = fileHead;
	while(itr != NULL){
		printf("%s %d \n",itr->name,itr->lock);
		itr = itr->next;
	}
}

void addList(char * name){
	fList * node = malloc(sizeof(fList));
	fList * ch = fileHead;
	//first time
	if(fileHead->next == NULL){
		strcpy(node->name,name);
		node->lock = 0;
		node->next = NULL;
		fileHead->next = node;
	}
	else{
		while(ch->next != NULL){
			ch = ch->next;
		}
		strcpy(node->name,name);
		node->lock = 0;
		node->next = NULL;
		ch->next = node;
	}
}

void removeList(fList* target){
	if(fileHead->next == NULL){
		printf("빈 리스트 \n");
		return ;
	}
	fList * pre = fileHead;
	fList * ch = fileHead->next;

	while(ch != NULL){
		if(ch == target && ch->lock == 0){
			pre->next = ch->next;
			free(ch);
			printf("삭제 성공\n");
			return;
		}else if(ch->lock != 0){
			printf("사용중\n");
			break;
		}
		else{
			pre = pre->next;
			ch = ch->next;
		}
	}
}

void freeList(fList** head){
	fList *itr;
	fList *pre;
	itr = *head;
	while(itr != NULL){
		pre = itr->next;
		free(itr);
		itr = pre;
	}
	*head = NULL;
}

int checkLock(char * target_file){
	fList * ch = findList(target_file);
	if(ch == NULL){
		return -1; // file not exist
	}
	return ch->lock;
}

int  lockList(char * target_file){
	fList * ch = findList(target_file);
	if( ch->lock == 0){
		ch->lock = 1;
		return 0;
	}
	else{
		return -1;
	}
}

void unlockList(char * target_file){
	fList * ch = findList(target_file);
	if( ch->lock == 1){
		ch->lock = 0;
		return 0;
	}
	else{
		return -1;
	}
}

void handle_sigint() {
  close(serv_sock);
  freeList(fileHead);
  printf("서버 종료됨\n");
  exit(1);
}

int sendMsg(int sock,char msg[]){
	if((send(sock,msg,strlen(msg)+1,0))==-1){
		fprintf(stderr,"(%d) can't packet\n",sock);
		return errno;
	}
	return 0;
}

//서버 쓰레드
void *server_thread(void *sock){
	int buffer_size = 1024;
	char *buffer = malloc(sizeof(char) * buffer_size);
	while (1){
		//명령어 입력확인
		if (recv((int)sock, buffer, buffer_size, 0) < 1){
			fprintf(stderr, "소켓(%d) 종료됨 \n", (int)sock);
			perror("recv() error");
			close((int)sock);
			break;
		}
		//서버 동작 구현
		server_process((int)sock, buffer);
	}
}

int cmdchk(const char *str, const char *pre){
	size_t lenpre = strlen(pre);
	size_t lenstr = strlen(str);
	return (lenstr < lenpre) ? 0 : memcmp(pre, str, lenpre) == 0;
}

//push request (send file to client)
int request_pull(int sock, char *target_file){
	FILE *fd;
	fList * ch = findList(target_file);
	if ((fd = fopen(target_file, "rb")) == NULL){
		sendMsg(sock, "@file open error");
		perror("fopen() error");
		return -1;
	}
	if( ch == NULL){
		
		addList(target_file);
		ch = findList(target_file);
		printList();
	}
	if(lockList(ch) != 0){
		sendMsg(sock, "!file is using");
		printf("lock failed : %s \n",ch->name);
		fclose(fd);
		return -1;
	}
	char buffer[1024];
	ssize_t chunk_size;
	fseek(fd, 0L, SEEK_END);
	sprintf(buffer, "%ld", ftell(fd));
	ssize_t byte_sent = send(sock, buffer, strlen(buffer) + 1, 0);
	if (byte_sent == -1){
		fprintf(stderr, "(%d) : can't send packet", sock);
		error_handling("send() error");
		fclose(fd);
		return -1;
	}
	fseek(fd, 0L, SEEK_SET);
	// Wait for client to be ready
	ssize_t byte_received = recv(sock, buffer, sizeof(buffer), 0);
	if (byte_received == -1){
		fprintf(stderr, "(%d) can't receive packet", sock);
		error_handling("recv() error");
		fclose(fd);
		return -1;
	}
	// Start Transmission
	while ((chunk_size = fread(buffer, 1, sizeof(buffer), fd)) > 0){
		ssize_t byte_sent = send(sock, buffer, chunk_size, 0);
		printf("%d %d\n",chunk_size,byte_received);
		if (byte_sent == -1){
			fprintf(stderr, "(%d) can't send packet", sock);
			error_handling("send() error");
			fclose(fd);
			return -1;
		}
	}
	printf("소켓 (%d) 으로  %s 전송완료\n", sock, target_file);
	fclose(fd);
	return 0;
}

//pull request (download from client)
int request_push(int sock, char *target_file){
	FILE *fd;
	if ((fd = fopen(target_file, "wb")) == NULL)
	{
		fprintf(stderr, "(%d) Can't open %s", sock, target_file);
		perror("");
		return -1;
	}


	if( findList(target_file) == NULL){	//파일 존재 하지 않을경우
		addList(target_file);
		printList();
	}
	lockList(findList(target_file));

	char buffer[1024];
	strcpy(buffer, "size?");
	ssize_t byte_sent = send(sock, buffer, strlen(buffer) + 1, 0);
	if (byte_sent == -1)
	{
		fprintf(stderr, "(%d) can't send packet", sock);
		perror("");
		fclose(fd);
		return -1;
	}

	ssize_t byte_received = recv(sock, buffer, sizeof(buffer), 0);
	if (byte_received == -1)
	{
		fprintf(stderr, "(%d) can't receive packet", sock);
		perror("");
		fclose(fd);
		return -1;
	}
	long file_size = strtol(buffer, NULL, 0); 

	strcpy(buffer, "ready");
	byte_sent = send(sock, buffer, strlen(buffer) + 1, 0);
	if (byte_sent == -1)
	{
		fprintf(stderr, "(%d) can't send packet", sock);
		perror("");
		fclose(fd);
		return -1;
	}

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
			return -1;
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
	fprintf(stderr, "소켓(%d): %s 저장완료\n", sock, target_file);
	unlockList(findList(target_file));
	fclose(fd);
	return 0;
}

int request_ls(int sock){
	if(fileHead->next == NULL){
		sendMsg(sock,"@nofiles!");
		return -1;
	}
	fList * ch = fileHead->next;
	char buffer[1024];
	//  클라 응답 체크
	strcpy(buffer,"ready");
	if (send(sock, buffer, sizeof(buffer), 0) == -1){
		perror("socket send fail()");
		return -1;
	}
	
	strcpy(buffer,"");
	while(ch != NULL){
		strcpy(buffer,ch->name);
		strcat(buffer,"-");
		if(ch->lock == 1) {strcat(buffer,"locked"); }
		else { strcat(buffer,"unlock");}
		printf("msg : (%s) \n",buffer);
		if(send(sock,buffer,sizeof(buffer),0) < 0){
			fprintf(stderr, "can't send buffer");
			//sendMsg(sock,"@error!");
			return -1;
		}
		ch = ch->next;
	}
	strcpy(buffer,"#EOF");
	send(sock,buffer,sizeof(buffer),0);
	return 0;
}

int request_rm(int sock, char * target_file){
	fList * ch = findList(target_file);
	if(ch == NULL){
		sendMsg(sock,"@nofiles!");
		return -1;
	}
	else if(ch->lock == 1){
		sendMsg(sock,"!filelocked");
		return -2;
	}
	else{
		if(unlink(target_file)!= 0){
			sendMsg(sock,"@error!");
			return errno;
		}
		removeList(ch);
		printList();
		sendMsg(sock,"#done!");
		return 0;
	}
	sendMsg(sock,"@someerrors!");
	return -1;
	
}

//서버의 주동작
//command를 받아 각 기능별 알맞는 처리를 함.
int server_process(int sock, char *command){
	char *blank = " ";
	char *cmd = strtok(command, blank);
	char *context = strtok(NULL, blank);
	char *response = malloc(sizeof(char) * 1024);
	int rs=0;
	printf("received msg from (%d) : %s \n",sock,cmd);
	if (cmdchk(cmd, "pull")){
		rs=request_pull(sock,context);
		//sendMsg(sock,response);
	}
	else if (cmdchk(cmd, "push")){
		rs=request_push(sock,context);
		//sendMsg(sock,response);
	}
	else if (cmdchk(cmd, "ls")){
		rs = request_ls(sock);
	}
	else if(cmdchk(cmd,"rm")){
		rs = request_rm(sock,context);
	}
	else{
		strcpy(response,"No such command:");
		strcat(response,command);
		sendMsg(sock,response);
	}
	free(response);
	return rs;
}

void error_handling(char *message){
	perror(message);
	exit(1);
}

int main(int argc, char *argv[]){
	int str_len;
	char buffer[1024];
	char buf[256];
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;

	fileHead = malloc(sizeof(fList));
	
	pthread_mutex_init(&mutex,NULL);


	if (argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET,SOCK_STREAM, 0);
	if (serv_sock == -1)
	{
		error_handling("socket() error");
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));
	printf("서버 포트 : %d \n", ntohs(serv_addr.sin_port));
	if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
	{
		error_handling("bind() error");
	}

	if (listen(serv_sock, MAX_CLNT) == -1)
	{
		error_handling("listen() error");
		close((int)clnt_sock);
		exit(errno);
	}
	clnt_addr_size = sizeof(clnt_addr);

	//ctrl+c 시 종료되도록 설정
	signal(SIGINT, handle_sigint);

	while(1){
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
		if ((int)clnt_sock == -1){
			perror("accept() error");
			continue;
		}
		
		printf("socket(%d) 접속\n",(int)clnt_sock);
		pthread_t thread;
		if (pthread_create(&thread, NULL, server_thread,clnt_sock)){
			printf("(%d) 쓰레드 생성에러\n",(int)clnt_sock);
		}
	}
	close((int)clnt_sock);
	close(serv_sock);
	freeList(fileHead);
	return 0;
}