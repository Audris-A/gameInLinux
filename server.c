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

int playerSymbol = 0;

int playerCount;

struct playerSockets *head_p;

//Player structure
struct player {
    char name[255];
    char symbol[2];
    int playerfd;

    struct player *next;
};

struct player *head;

int checkExistingName(char *name){
    struct player *p = head;

    while (p) {
        if(strcmp(p->name, name) == 0){
            return 1;
        }

        p = p->next;
    }
        
    return 0;
}

//Izveidot jaunu mezglu player sarakstā
void insert_player(char *name, int playerfd) 
{   
    int i = 65 + playerSymbol;
    char text[2];

    text[0] = (char)i;
    text[1] = '\0';

    //Izveido pašu elementu
    struct player *elem = (struct player*) malloc(sizeof(struct player));

    strcpy(elem->name, name);

    elem->playerfd = playerfd;

    strcpy(elem->symbol, text);

    fprintf(stderr, "Symbol - %s\n", elem->symbol);

    playerSymbol++;

    //Izveido norādi uz iepriekšējo pirmo elementu
    elem->next = head;

    //Norāda, ka jaunizveidotais elements ir pirmais
    head = elem;
};

//Izvadīt ListElem sarakstu
struct player *outputList() {
    struct player *p = head;
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
    struct player *p = head;
    char *plCnt = malloc(sizeof(char) * 3);
    char *info = malloc(sizeof(char) * 256);
    unsigned int plLen = 0;
    strcpy(info, "2Players - ");

    outputList();
    sprintf(plCnt, "%d", playerCount);

    strcat(info, plCnt);

    strcat(info, " [");

    struct player *n = head;

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
    int received = -1;

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
        //If everything is ok
        insert_player(mBuff, sock);
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

    printf("%s:%d connected via TCP\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
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
    if (bind(serversock, (struct sockaddr *) &echoserver, sizeof(echoserver)) < 0) {
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

        //Output client info
        printClient(clientFd);      

        //Create client thread
        pthread_t clientThread;
        pthread_create(&clientThread, NULL, HandleClient, (void *) clientFd);
    }

    printf("In main thread");
    while(0){
       //Sit here or a while.
    }
}