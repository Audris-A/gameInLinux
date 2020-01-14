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

int playerCount;

//Player structure
struct player {
    char name[255];
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

    //Izveido norādi uz iepriekšējo pirmo elementu
    elem->next = head;

    //Norāda, ka jaunizveidotais elements ir pirmais
    head = elem;
};

struct player *refreshPlayerCount() {
    struct player *p = head;
    playerCount = 0;

    while (p) {
        playerCount++;
        p = p->next;
    }

    return NULL;
};

void Die(char *mess) { perror(mess); exit(1); }

char *getPlayerUsernames() {
    char *info = malloc(sizeof(char) * 256);
    strcpy(info, "{");
    struct player *n = head;

    while (n) {
        strcat(info, n->name);

        n = n->next;
    }

    strcat(info, "}");

    return info;
}

void lobbyInfo() {
    struct player *p = head;
    char *plCnt = malloc(sizeof(char) * 3);
    char *info = malloc(sizeof(char) * 256);
    unsigned int plLen = 0;

    strcpy(info, "2");

    refreshPlayerCount();

    sprintf(plCnt, "%d", playerCount);

    strcat(info, plCnt);

    strcat(info, getPlayerUsernames());

    strcat(info, "\0");

    plLen = strlen(info);

    while(p){
        if (send(p->playerfd, info, plLen, 0) != plLen) {
            Die("Refresh mismatch:");
        }

        p = p->next;
    }
}

void handleClient(int sock) {
    char *mBuff = malloc(sizeof(char) * 255);
    int received = -1;

    // Receive username
    if ((received = recv(sock, mBuff, BUFFSIZE, 0)) < 0) {
        Die("Failed to receive initial bytes from client");
    }

    if (mBuff[0] == '0') {
        memmove(mBuff, mBuff+1, (strlen(mBuff) + 1));
    } else {
        // Bad message error
        char *msg = malloc(sizeof(char) * 255);
        int msgLen = 0;

        strcpy(msg, "Undefined message!\0");

        msgLen = strlen(msg);

        if (send(sock, msg, msgLen, 0) != msgLen) {
            Die("Undefined message error:");
        }

        free(msg);
        close(sock);
    }

    // Username validation.
    if(checkExistingName(mBuff) == 1){
        if (send(sock, "4\0", 2, 0) != 2) {
            Die("Existing username mismatch:");
        }

        close(sock);
    } else {
        // If everything is ok
        insert_player(mBuff, sock);
    }

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

// Map row sending to client
void sendRow(char *fileName, int lineLength) {
    FILE *input = fopen(fileName, "r");
    char *line = NULL;
    int rowCount = 1;

    size_t len = 0;
    ssize_t read;

    if (input == NULL) {
        perror("No file:");
        exit(0);
    }

    // Digest map row
    while ((read = getline(&line, &len, input)) != -1) {
        char *infoHolder = malloc(sizeof(char) * 4);

        char *mBuff = malloc(sizeof(char) * (lineLength + 4));

        // Add message type and row number
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

        while(p){
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

    for(;;){
        //
    }
}

void gameStart(char *fileName) {
    FILE *input = fopen(fileName, "r");
    char * line = NULL;
    int rowCount = 0;
    int gotLineLength = 0;
    int lineLength = 0;
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

        if (gotLineLength == 0) {
            lineLength = strlen(line);
            gotLineLength = 1;
        }
    }

    fclose(input);

    // Message type
    strcpy(mBuff, "5");
    strcat(mBuff, infoHolder);

    // Player usernames
    strcat(mBuff, getPlayerUsernames());

    // Map x coordinate
    sprintf(infoHolder, "%ld", strlen(line));

    if (strlen(infoHolder) == 1) {
        strcat(mBuff, "00");
    } else if (strlen(infoHolder) == 2) {
        strcat(mBuff, "0");
    }

    strcat(mBuff, infoHolder);

    // Map y coordinate
    sprintf(infoHolder, "%d", rowCount);

    if (strlen(infoHolder) == 1) {
        strcat(mBuff, "00");
    } else if (strlen(infoHolder) == 2) {
        strcat(mBuff, "0");
    }

    strcat(mBuff, infoHolder);

    strcat(mBuff, "\0");

    // Send GAME_START information to clients
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

    sendRow(fileName, lineLength);
}

int main(int argc, char *argv[]) {
    int serversock; 
    struct sockaddr_in server;
    int threadJoinStatus = 0;

    if (argc < 3) {
        fprintf(stderr, "USAGE: server <port> <path to map>\n");
        exit(1);
    }

    // Create the TCP socket
    if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        Die("Failed to create socket");
    }

    // Construct the server sockaddr_in structure
    memset(&server, 0, sizeof(server));           // Clear struct
    server.sin_family = AF_INET;                  // Internet/IP
    server.sin_addr.s_addr = htonl(INADDR_ANY);   // Incoming addr
    server.sin_port = htons(atoi(argv[1]));       // server port 

    // Bind the server socket
    if (bind(serversock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        Die("Failed to bind the server socket");
    }

    // Listen on the server socket
    if (listen(serversock, MAXPENDING) < 0) {
        Die("Failed to listen on server socket");
    }

    // Accept new clients
    for (;;) {
        struct sockaddr_in peerAddr;
        socklen_t addrSize = sizeof(peerAddr);

        int clientFd = accept(serversock, (struct sockaddr *) &peerAddr, &addrSize);

        if (clientFd == -1) {
            perror("accept");
            return -1;
        }

        // Output client info
        printClient(clientFd);

        // Create client thread
        pthread_t clientThread;
        pthread_create(&clientThread, NULL, handleClient, (void *) clientFd);
        pthread_join(clientThread, threadJoinStatus);

        refreshPlayerCount();
        lobbyInfo();

        if (threadJoinStatus != 0) {
            fprintf(stderr, "Join error %d\n", threadJoinStatus);
        }

        // Accept clients for 10 seconds after player count is bigger than 2
        if (playerCount > 1) {
            fd_set rfds;
            struct timeval tv;
            int retval;

            tv.tv_sec = 10;
            tv.tv_usec = 0;

            for(;;){
                if (playerCount > 7) {
                    // Start the game because we already have 8 players connected
                    gameStart(argv[2]);
                }

                retval = select(serversock + 1, &rfds, NULL, NULL, &tv);

                if (retval == -1){
                    perror("Bad select():");
                } else if (retval){
                    // Data is available
                    int clientFd = accept(serversock, (struct sockaddr *) &peerAddr, &addrSize);

                    // Output client info
                    printClient(clientFd);

                    // Create client thread
                    pthread_t clientThread;
                    pthread_create(&clientThread, NULL, handleClient, (void *) clientFd);
                    pthread_join(clientThread, threadJoinStatus);

                    refreshPlayerCount();
                    lobbyInfo();
                } else {
                    // Timer ended, so start the game
                    gameStart(argv[2]);
                }
            }
        }
    }
}