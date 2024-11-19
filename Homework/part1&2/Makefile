CC = gcc
CFLAGS = -lm
TARGETS = server client

all: $(TARGETS)

server: udpServer.o
	$(CC) -o server udpServer.o $(CFLAGS)

client: udpClient.o
	$(CC) -o client udpClient.o $(CFLAGS)

udpServer.o: udpServer.c
	$(CC) -c udpServer.c $(CFLAGS)

udpClient.o: udpClient.c
	$(CC) -c udpClient.c $(CFLAGS)

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
