//서버
//데이터 송/수신용 소켓 _sock
//메세지 송/수신용 소켓 _msg_cock

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#define DATA_SOCK 0
#define MSG_SOCK 1

#define BACKLOG 5
#define MAX_CMD 100
char message[MAX_CMD];

void error_handling(char *message);

int main(int argc, char *argv[])
{
	int serv_sock[2];
	int clnt_sock[2];
	int str_len;
	char buf[256];
	struct sockaddr_in serv_addr[2];
	struct sockaddr_in clnt_addr[2];
	socklen_t clnt_addr_size[2];

	strcpy(message,"hello world!");
	
	if(argc!=2){
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	//init socks
	for(int i = 0; i < 1; i++){
		serv_sock[i]=socket(PF_INET, SOCK_STREAM, 0);

		if(serv_sock[i] == -1 ){
			printf("%d \n ",i);
			error_handling("socket() error");
		}
		printf("%d socket succeed \n ",i);
		memset(&serv_addr[i], 0, sizeof(serv_addr[i]));
		serv_addr[i].sin_family=AF_INET;
		serv_addr[i].sin_addr.s_addr=htonl(INADDR_ANY);
		serv_addr[i].sin_port=htons(atoi(argv[1])+i);
		printf("serv_addr[%d] port : %d",i,atoi(argv[1])+i);

		if(bind(serv_sock[i], (struct sockaddr*) &serv_addr[i], sizeof(serv_addr[i]))==-1 ){
			printf("%d \n ",i);
			error_handling("bind() error");
		} 
		printf("%d bind() succeed \n ",i);

		if(listen(serv_sock[i], 5)==-1){
			printf("%d \n ",i);
			error_handling("listen() error");
		}
		printf("%d listen() succeed \n ",i);

		clnt_addr_size[i]=sizeof(clnt_addr[i]);  
		clnt_sock[i]=accept(serv_sock[i], (struct sockaddr*)&clnt_addr[i],&clnt_addr_size[i]);
		if(clnt_sock[i]==-1){
			printf("%d \n ",i);
			error_handling("accept() error");
		}
		printf("%d accept() succeed \n ",i);
	}
	 
	
	write(clnt_sock[DATA_SOCK], message, strlen(message)-1);
	int nbyte = 256;
    size_t filesize = 0, bufsize = 0;
    FILE *file = NULL;
   	file = fopen("test.txt", "wb");
   	bufsize = 256;
   	while(nbyte!=0) {
       	nbyte = recv(clnt_sock[DATA_SOCK], buf, bufsize, 0);
       	fwrite(buf, sizeof(char), nbyte, file);		
   	}
	fclose(file);
	printf("file update complete!\n");
	while(1){
		str_len=read(clnt_sock[DATA_SOCK], message, BUFSIZ-1);
		if(str_len==-1){error_handling("read() error!");}
		if(!strcmp(message,"q\n") || !strcmp(message,"Q\n")){
			break;
		}
		write(clnt_sock[DATA_SOCK], message, strlen(message));
		message[str_len]=0;
		printf("Message from client MSG_SOCK: %s \n", message);

		
	}
	for(int i = 0; i < 2 ; i ++){
		close(clnt_sock[i]);	
		close(serv_sock[i]);
	}
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
