.PHONY: all server client clean

all: server client

server:
	$(CC) -g -Wall server.c -o server -lpthread

client:
	$(CC) -g -Wall client.c -o client -lpthread

clean:
	rm -f server client
