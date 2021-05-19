CC = gcc

TARGET = server client

all : server.o

server.o : server.c
	$(CC) -c -o server.o server.c

clean :
	rm *.o $(TARGET)
