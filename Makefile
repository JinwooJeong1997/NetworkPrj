CC = gcc

TARGET = server client

all : server.o client.o
	$(CC) -o server server.o
	$(CC) -o client client.o

server.o : server.c
	$(CC) -c -o server.o server.c

client.o : client.c
	$(CC) -c -o client.o client.c

clean :
	rm *.o $(TARGET)
