CC = gcc
CFLAGS = -Wall -Wextra -pthread -I.
LDFLAGS = -pthread

all: client server

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c -o common.o

client: client.c common.o
	$(CC) $(CFLAGS) -o client client.c common.o $(LDFLAGS)

server: server.c common.o
	$(CC) $(CFLAGS) -o server server.c common.o $(LDFLAGS)

clean:
	rm -f client server common.o