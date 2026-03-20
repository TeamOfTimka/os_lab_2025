CC = gcc
CFLAGS = -Wall

all: tcpserver tcpclient udpserver udpclient

tcpserver: tcpserver.c
	$(CC) $(CFLAGS) -o tcpserver tcpserver.c

tcpclient: tcpclient.c
	$(CC) $(CFLAGS) -o tcpclient tcpclient.c

udpserver: udpserver.c
	$(CC) $(CFLAGS) -o udpserver udpserver.c

udpclient: udpclient.c
	$(CC) $(CFLAGS) -o udpclient udpclient.c

clean:
	rm -f tcpserver tcpclient udpserver udpclient

.PHONY: all clean