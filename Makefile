.PHONY: all server client clean

all: server client

server:
	$(CC) -g -Wall server.c common.c -o server -lpthread

client:
	$(CC) -g -Wall client.c common.c -o client

clean:
	rm -f server client
