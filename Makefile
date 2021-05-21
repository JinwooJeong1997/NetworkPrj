CC = gcc

TARGET = server client


all : server.o client.o
	$(CC)  -o server server.o -lpthread
	$(CC) -o client client.o -lpthread

server.o : server.c
	$(CC) -c -o server.o server.c -lpthread

client.o : client.c
	$(CC) -c -o client.o client.c -lpthread

clean :
	rm *.o $(TARGET)
