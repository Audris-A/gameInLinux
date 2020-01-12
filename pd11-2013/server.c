#include "common.h"
#include <pthread.h>

// listen() funkcijas paramters - cik daudz *jaunu* konekciju akceptēt vienlaicīgi?
#define LISTEN_BACKLOG 5

// ----------------------------------

// ECHO komandas apstrāde
void handleEchoCommand(int fd, uint8_t *data, unsigned length, struct sockaddr_in *remote)
{
    printf("ECHO\n");
    if (length == 0) {
	fprintf(stderr, "handle echo: no data passed!\n");
	return;
    }
    // nosūta atpakaļ (atbalso) saņemtos datus
    sendData(fd, (char *)data, length, remote);
}

// GETHOSTNAME komandas apstrāde
void handleGethostnameCommand(int fd, unsigned length,
	                      struct sockaddr_in *remote)
{
    printf("GETHOSTNAME\n");
    if (length != 0) {
	fprintf(stderr, "handle gethostname: warning - unexpected data passed!\n");
    }

    char buffer[100];	
    if (gethostname(buffer, sizeof(buffer) - 1) < 0) {
	perror("gethostname");
	return;
    }

    // nosūta hosta vārdu
    sendData(fd, buffer, strlen(buffer) + 1, remote);
}

void handleData(int fd, NetCommand_t *command, struct sockaddr_in *remote) {
    switch (command->command) {
    case CMD_ECHO:
	handleEchoCommand(fd, command->data, command->length - sizeof(NetCommand_t), remote);
	break;
    case CMD_GETHOSTNAME:
	handleGethostnameCommand(fd, command->length - sizeof(NetCommand_t), remote);
	break;
    default:
	fprintf(stderr, "handleData: unknown command %u!\n",
		command->command);
	break;
    }
}

// izdrukā klienta informāciju
void printClient(int fd) {
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);

    if (getpeername(fd, (struct sockaddr *) &addr, &addrLen) == -1) {
	perror("getpeername");
	return;
    }

    printf("%s:%d connected via TCP\n",
	    inet_ntoa(addr.sin_addr),
	    ntohs(addr.sin_port));
}


// TCP klienta datu apstrādes funkcija (tiek izsaukta jaunā pavedienā)
void *clientHandler(void *param)
{
    int clientFd = (int) param;
    char buffer[MAX_MESSAGE];

    for (;;) {
	NetCommand_t *command = (NetCommand_t *) buffer;

	int len = recv(clientFd, command, sizeof(NetCommand_t), 0);
	if (len < 0) {
	    perror("recv TCP");
	    return NULL;
	}
	if (len < sizeof(NetCommand_t)) {
	    fprintf(stderr, "EOF on TCP\n");
	    return NULL;
	}

	if (command->length > sizeof(NetCommand_t)) {
	    if (command->length > MAX_MESSAGE) {
		fprintf(stderr, "malformed packet on TCP!\n");
		return NULL;
	    }
	    int dataLen = recv(clientFd, command->data, command->length - sizeof(NetCommand_t), 0);
	    if (dataLen < 0) {
		perror("recv TCP");
		return NULL;
	    }
	    if (dataLen < command->length - sizeof(NetCommand_t)) {
		fprintf(stderr, "EOF on TCP\n");
		return NULL;
	    }
	}

	handleData(clientFd, command, NULL);
    }
}

// UDP klienta datu apstrādes funkcija (tiek izsaukta jaunā pavedienā)
void *udpHandler(void *param)
{
    int fd = (int) param;
    char buffer[MAX_MESSAGE];

    for (;;) {
	struct sockaddr_in peerAddr;
	socklen_t peerAddrSize = sizeof(peerAddr);

	int len = recvfrom(fd, buffer, MAX_MESSAGE, 0,
		(struct sockaddr *) &peerAddr, &peerAddrSize);
	if (len < 0) {
	    perror("recv udp");
	    return NULL;
	}

	printf("received %d bytes from %s:%d via UDP\n",
		len,
		inet_ntoa(peerAddr.sin_addr),
		ntohs(peerAddr.sin_port));

	if (len < sizeof(NetCommand_t)) {
	    fprintf(stderr, "command too short!\n");
	    continue;
	}

	NetCommand_t *command = (NetCommand_t *) buffer;
	if (command->length > MAX_MESSAGE || command->length < sizeof(NetCommand_t)) {
	    fprintf(stderr, "malformed packet on UDP!\n");
	    return NULL;
	}
	if (len < command->length - sizeof(NetCommand_t)) {
	    fprintf(stderr, "EOF on UDP!\n");
	    return NULL;
	}
	handleData(fd, command, &peerAddr);
    }
}

int main(void)
{
    // izveidojamm TCP soketu
    int tcpFd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpFd < 0) {
	perror("tcp socket");
	return -1;
    }

    // izveidojamm UDP soketu
    int udpFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpFd < 0) {
	perror("udp socket");
	return -1;
    }

    // pasakam uz kādu adresi & port (š.g. tikai portu) klausīties
    struct sockaddr_in myAddr;
    memset(&myAddr, 0, sizeof(struct sockaddr_in));
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(PORT);
    if (bind(tcpFd, (struct sockaddr *) &myAddr, sizeof(struct sockaddr_in)) == -1) {
	perror("tcp bind");
	return -1;
    }
    if (bind(udpFd, (struct sockaddr *) &myAddr, sizeof(struct sockaddr_in)) == -1) {
	perror("udp bind");
	return -1;
    }

    // izveidojam UDP soketam savu pavedienu
    pthread_t udpThread;
    pthread_create(&udpThread, NULL, udpHandler, (void *) udpFd);		

    // klausāmies uz konekcijām 
    if (listen(tcpFd, LISTEN_BACKLOG) == -1) {
	perror("listen");
	return -1;
    }

    // cikls, kurā uz TCP soketa tiek akceptēti jauni klienti
    for (;;) {
	struct sockaddr_in peerAddr;
	socklen_t addrSize = sizeof(peerAddr);
	int clientFd = accept(tcpFd, (struct sockaddr *) &peerAddr, &addrSize);
	if (clientFd == -1) {
	    perror("accept");
	    return -1;
	}

	// klients pievienojies, izdrukāju
	printClient(clientFd);		

	// izveidojam savu pavedienu šim klientam
	pthread_t clientThread;
	pthread_create(&clientThread, NULL, clientHandler, (void *) clientFd);
	/* pthread_t *clientThread = malloc(sizeof(pthread_t)); */
	/* pthread_create(clientThread, NULL, clientHandler, (void *) clientFd); */
    }
}
