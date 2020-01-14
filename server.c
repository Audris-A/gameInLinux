/*
    Read with 0 username message
    username format fix 0name\0\0\0\0\0...
*/

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

int startCountdown = 0;

int countdownEnded;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    //pthread_mutex_lock(&mutex);
    // pthread_mutex_unlock(&mutex);
int playerSymbol = 0;

int playerCount;

struct playerSockets *head_p;

//Player structure
struct player {
    char name[255];
    //char symbol[2];
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
    //Izveido pašu elementu
    struct player *elem = (struct player*) malloc(sizeof(struct player));

    strcpy(elem->name, name);

    elem->playerfd = playerfd;

    //strcpy(elem->symbol, text);

    //fprintf(stderr, "Symbol - %s\n", elem->symbol);

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

char *getPlayers() {
    char *info = malloc(sizeof(char) * 256);
    strcpy(info, "{");
    struct player *n = head;

    while (n) {
        strcat(info, n->name);
        fprintf(stderr, "%s\n", n->name);

        n = n->next;
    }

    strcat(info, "}");

    return info;
}

void refreshPlayerCount() {
    struct player *p = head;
    char *plCnt = malloc(sizeof(char) * 3);
    char *info = malloc(sizeof(char) * 256);
    unsigned int plLen = 0;
    strcpy(info, "2");

    outputList();
    sprintf(plCnt, "%d", playerCount);

    strcat(info, plCnt);

    strcat(info, getPlayers());

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

    if (mBuff[0] == '0') {
        memmove(mBuff, mBuff+1, (strlen(mBuff) + 1));
    } else {
        //Bad message error
        close(sock);
    }

    //Username validation.
    if(checkExistingName(mBuff) == 1){
        //fprintf(stderr, "BAD\n");
        if (send(sock, "4\0", 2, 0) != 2) {
            Die("Existing username mismatch:");
        }

        close(sock);
    } else {
        //If everything is ok
        insert_player(mBuff, sock);
        outputList();
        refreshPlayerCount();
    }// if game has started!!!!!!!!!!!!!

    free(mBuff);
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

//Pa rindai karti nosūtīt
void sendRow(char *fileName) {
    FILE *input = fopen(fileName, "r");
    char *line = NULL;
    int rowCount = 1;

    size_t len = 0;
    ssize_t read;

    if (input == NULL) {
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, input)) != -1) {
    //while(fgets(line, 92, input)!= NULL) {
        //fprintf(stdout, "Linija %s garums - %ld\n", line, len);
        char *infoHolder = malloc(sizeof(char) * 4);

        char *mBuff = malloc(sizeof(char) * 97);

        strcpy(mBuff, "6");

        sprintf(infoHolder, "%d", rowCount);


        if (strlen(infoHolder) == 1) {
            strcat(mBuff, "00");
        } else if (strlen(infoHolder) == 2) {
            strcat(mBuff, "0");
        }

        strcat(mBuff, infoHolder);

        struct player *p = head;
        unsigned int plLen = 0;

        strcat(mBuff, line);
        strcat(mBuff, "\0");
        plLen = strlen(mBuff);

        fprintf(stderr, "> %s\n", mBuff);

        while(p){
            //send_all(p->playerfd, (void *) mBuff, 96);
            if (send(p->playerfd, mBuff, plLen, 0) != plLen) {
                Die("Refresh mismatch:");
            }

            p = p->next;
        }

        free(infoHolder);
        free(mBuff);

        rowCount++;
    }

    fclose(input);

    /*if (line) {
        free(line);
    }*/

    for(;;){
        //
    }
}

void gameStart(char *fileName) {
    FILE *input = fopen(fileName, "r");
    char * line = NULL;
    int rowCount = 0;

    size_t len = 0;
    ssize_t read;

    char *infoHolder = malloc(sizeof(char) * 10);

    char *mBuff = malloc(sizeof(char) * 255);

    sprintf(infoHolder, "%d", playerCount);


    if (input == NULL) {
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, input)) != -1) {
        rowCount++;
    }

    fclose(input);

    //Message type
    strcpy(mBuff, "5");
    strcat(mBuff, infoHolder);

    //Player usernames
    strcat(mBuff, getPlayers());

    //Map x coordinate
    sprintf(infoHolder, "%ld", strlen(line));

    if (strlen(infoHolder) == 1) {
        strcat(mBuff, "00");
    } else if (strlen(infoHolder) == 2) {
        strcat(mBuff, "0");
    }

    strcat(mBuff, infoHolder);

    //Map y coordinate
    sprintf(infoHolder, "%d", rowCount);

    if (strlen(infoHolder) == 1) {
        strcat(mBuff, "00");
    } else if (strlen(infoHolder) == 2) {
        strcat(mBuff, "0");
    }

    strcat(mBuff, infoHolder);

    //Send GAME_START information to clients
    struct player *p = head;
    unsigned int plLen = 0;

    plLen = strlen(mBuff);

    while(p){
        if (send(p->playerfd, mBuff, plLen, 0) != plLen) {
            Die("Refresh mismatch:");
        }

        p = p->next;
    }

    free(infoHolder);
    free(mBuff);

    sendRow(fileName);
}

int main(int argc, char *argv[]) {
    int serversock; //clientsock;
    struct sockaddr_in echoserver; //echoclient;
    int joinStatus = 0;
    countdownEnded = 0;

    if (argc < 3) {
        fprintf(stderr, "USAGE: echoserver <port> <path to map>\n");
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
        pthread_join(clientThread, joinStatus);

        if (joinStatus != 0) {
            fprintf(stderr, "Join error %d\n", joinStatus);
        }

        if (playerCount > 1) {
            fd_set rfds;
            struct timeval tv;
            int retval;

            tv.tv_sec = 10;
            tv.tv_usec = 0;

            for(;;){
                if (playerCount > 7) {
                    //Start game!!!!
                    fprintf(stderr, "Start game from count\n");
                    gameStart(argv[3]);
                }

                retval = select(serversock + 1, &rfds, NULL, NULL, &tv);
                //fprintf(stderr, "Retval - %d\n", retval);
                /* Don't rely on the value of tv now! */

                if (retval == -1){
                    perror("select()");
                } else if (retval){
                    printf("Data is available now.\n");
                    int clientFd = accept(serversock, (struct sockaddr *) &peerAddr, &addrSize);

                    //Output client info
                    printClient(clientFd);

                    //Create client thread
                    pthread_t clientThread;
                    pthread_create(&clientThread, NULL, HandleClient, (void *) clientFd);
                    pthread_join(clientThread, joinStatus);
                } else {
                    fprintf(stderr, "Start game from time\n");
                    //Start game!
                    gameStart(argv[2]);
                }
            }
        }
    }
}