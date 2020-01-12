#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAXPENDING 30    /* Max connection requests */
#define BUFFSIZE 100
#define MAX_MESSAGE 1000

int playerCount;

//Username structure
struct playerSockets {
	int playerfd;

	struct playerSockets *next;
};

struct playerSockets *head_p;

//Username structure
struct userNames {
	char name[255];
	char symbol[2];

	struct userNames *next;
};

struct userNames *head;

int checkExistingName(char *name){
	struct userNames *p = head;

	while (p) {
		//fprintf(stderr, "%s un %s\n", p->name, user);
		if(strcmp(p->name, name) == 0){
			return 1;
		}

		p = p->next;
	}
		
	return 0;
}

//Izveidot jaunu mezglu playerSockets sarakstā
void insert_socket(int playerfd) 
{
	//Izveido pašu elementu
	struct playerSockets *elem = (struct playerSockets*) malloc(sizeof(struct playerSockets));

	elem->playerfd = playerfd;

    //Izveido norādi uz iepriekšējo pirmo elementu
   	elem->next = head_p;

   	//Norāda, ka jaunizveidotais elements ir pirmais
   	head_p = elem;
};

//Izveidot jaunu mezglu userNames sarakstā
void insert_username(char *name) 
{
	//Izveido pašu elementu
	struct userNames *elem = (struct userNames*) malloc(sizeof(struct userNames));

	strcpy(elem->name, name);
	//elem->name[strlen(elem->name)+1] = '\0';
	//fprintf(stderr, " length   %ld\n", strlen(elem->name));
	//strcat(elem->name, '\0');
    //Izveido norādi uz iepriekšējo pirmo elementu
   	elem->next = head;

   	//Norāda, ka jaunizveidotais elements ir pirmais
   	head = elem;
};

//Izvadīt ListElem sarakstu
struct userNames *outputList() {
	struct userNames *p = head;
	playerCount = 0;

	while (p) {
		//printf("%s\n", p->name);
		playerCount++;
		p = p->next;
	}
	
	//printf("Players - %d\n", playerCount);

	return NULL;
};

void Die(char *mess) { perror(mess); exit(1); }

void refreshPlayerCount() {
	struct playerSockets *p = head_p;
	char *plCnt = malloc(sizeof(char) * 3);
	char *info = malloc(sizeof(char) * 256);
	unsigned int plLen = 0;
	strcpy(info, "2Players - ");

	outputList();
	sprintf(plCnt, "%d", playerCount);

	strcat(info, plCnt);

	strcat(info, " [");

	struct userNames *n = head;

	while (n) {
		strcat(info, n->name);

		if (n->next != NULL) {
			strcat(info, ", ");
		}

		n = n->next;
	}

	strcat(info, "]");

	plLen = strlen(info);

	while(p){
		if (send(p->playerfd, info, plLen, 0) != plLen) {
	      	Die("Refresh mismatch:");
		}

		p = p->next;
	}
}

void HandleClient(int sock) {
	char *mBuff = malloc(sizeof(char) * 255);
	//char *plCnt = malloc(sizeof(char) * 3);
	//unsigned int plLen = 0;
	int received = -1;
	//int check;

	/* Receive username */
	if ((received = recv(sock, mBuff, BUFFSIZE, 0)) < 0) {
	  	Die("Failed to receive initial bytes from client");
	}

	//Username validation.
	//mBuff[strlen(mBuff) + 1] = '\0';
	if(checkExistingName(mBuff) == 1){
		//fprintf(stderr, "BAD\n");
		close(sock);
	} else {
		insert_socket(sock);

		//If everything is ok
		insert_username(mBuff);
		outputList();
		refreshPlayerCount();

	    free(mBuff);
		//close(sock);
		while(playerCount != 30){
			//
		}
	}// if game has started!!!!!!!!!!!!!
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

int main(int argc, char *argv[]) {
    int serversock; //clientsock;
    struct sockaddr_in echoserver; //echoclient;

    //int statTh = 0;
    if (argc != 2) {
      fprintf(stderr, "USAGE: echoserver <port>\n");
      exit(1);
    }
    /* Create the TCP socket */
    if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      Die("Failed to create socket");
    }
    /* Construct the server sockaddr_in structure */
    memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
    echoserver.sin_family = AF_INET;                  /* Internet/IP */
    echoserver.sin_addr.s_addr = htonl(INADDR_ANY);   /* Incoming addr */
    echoserver.sin_port = htons(atoi(argv[1]));       /* server port */

	/* Bind the server socket */
	if (bind(serversock, (struct sockaddr *) &echoserver,
	                           sizeof(echoserver)) < 0) {
	Die("Failed to bind the server socket");
	}
	/* Listen on the server socket */
	if (listen(serversock, MAXPENDING) < 0) {
	Die("Failed to listen on server socket");
	}


	// cikls, kurā uz TCP soketa tiek akceptēti jauni klienti
    for (;;) {
		struct sockaddr_in peerAddr;
		socklen_t addrSize = sizeof(peerAddr);
		int clientFd = accept(serversock, (struct sockaddr *) &peerAddr, &addrSize);

		if (clientFd == -1) {
		    perror("accept");
		    return -1;
		}

		// klients pievienojies, izdrukāju
		printClient(clientFd);		

		// izveidojam savu pavedienu šim klientam
		pthread_t clientThread;
		pthread_create(&clientThread, NULL, HandleClient, (void *) clientFd);
		//pthread_join(clientThread, (void *) statTh);
		//pthread_exit(clientThread);
		//fprintf(stdout, "Status - %d\n", statTh);

		/* pthread_t *clientThread = malloc(sizeof(pthread_t)); */
		/* pthread_create(clientThread, NULL, clientHandler, (void *) clientFd); */
    }

    printf("In main thread");
    while(0){
    	//Sit here or a while.
    }
}