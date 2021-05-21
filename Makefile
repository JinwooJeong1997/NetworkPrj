CC = gcc

TARGET = server client
FLAG = -lpthread

all : server.o client.o
	$(CC) $(FLAG) -o server server.o
	$(CC) $(FLAG) -o client client.o

server.o : server.c
	$(CC) $(FLAG) -c -o server.o server.c

client.o : client.c
	$(CC) $(FLAG) -c -o client.o client.c

clean :
	rm *.o $(TARGET)
