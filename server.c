#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>

#define MAXPENDING 30    /* Max connection requests */
#define BUFFSIZE 100

extern int errno ;

int playerCount;
int serversock;
int closefd = 0;

char** map;

//Player structure
struct player {
    char name[17];
    int playerfd;
    int x;
    int y;
    int score;
    char symbol;
    int dead;
    int status;

    struct player *next;
};

struct player *head;

char symbols[8] = {'A','B','C','D','E','F','G','H'};
int symbol_it = 0;

int starting_positions[8][2] = {
        {1, 1},
        {89, 29},
        {34,13},
        {1,29},
        {88,1},
        {1,14},
        {89,14},
        {44,1}
    };


int food_count = 16;
int food_positions[16][3] = {
        {13, 7, 0}, // x, y, eaten
        {40, 7, 0},
        {83, 7, 0},
        {4, 13, 0},
        {21, 13, 0},
        {42, 13, 0},
        {61, 13, 0},
        {81, 13, 0},
        {7, 17, 0},
        {46, 17, 0},
        {76, 19, 0},
        {1, 23, 0},
        {14, 22, 0},
        {41, 23, 0},
        {71, 22, 0},
        {24, 29, 0}
    };

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

    elem->x = starting_positions[playerCount][0];
    elem->y = starting_positions[playerCount][1];

    elem->score = 0;

    elem->symbol = symbols[symbol_it];

    elem->dead = 0;

    elem->status = 0;

    symbol_it++;

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
        if (n) {
            strcat(info, ":");
        }
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
    pthread_exit(NULL);
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

    int row_it = 0;

    // Digest map row
    while ((read = getline(&line, &len, input)) != -1) {
        char *infoHolder = malloc(sizeof(char) * 4);

        char *mBuff = malloc(sizeof(char) * (lineLength + 4));

        char* row = (char*)malloc(sizeof(char)*lineLength);

        strcpy(row, line);

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

        map[row_it] = row;
        row_it++;

        strcat(mBuff, line);
        strcat(mBuff, "\0");
        plLen = strlen(mBuff);

        usleep(10000);

        while(p){
            //printf("%s", mBuff);
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

    map = (char**)malloc(sizeof(char*)*rowCount);

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

    int itt = 0;
    for (; itt< rowCount; itt++){
        printf("%s", map[itt]);
    }

    game_update();
}

int check_for_found_food(int x, int y){
    int k = 0;
    for (; k < food_count; k++){
        if (food_positions[k][2] != 1 && food_positions[k][0] == x && food_positions[k][1] == y){
            food_positions[k][2] = 1; 
            return 1;
        }
    }

    return 0;
}

int get_players_alive(){
    struct player *p = head;
    int cnt = 0;

    while(p){
        if (p->dead == 0){
            cnt++;
        }

        p = p->next;
    }

    return cnt;
}

void check_if_a_player_has_won(){
    struct player *p = head;
    char *mBuff = malloc(sizeof(char) * 5);

    // Inform the winner
    while(p){
        if (p->dead == 0){
            mBuff[0] = '9';
            mBuff[1] = (p->score)/10 + 48;
            mBuff[2] = ((p->score) % 10) + 48;

            mBuff[3] = 1 + 48;

            mBuff[4] = '\0';

            printf("sending to the winner %s \n", mBuff);

            if (send(p->playerfd , mBuff, 5, 0) != 5) {
                Die("check_if_a_player_has_won mismatch:");
            }

            break;
        }

        p = p->next;
    }

    free(mBuff);

    // End game
    game_end();
}

// returns the score to be added to the pl_fd player from set_new_coordinates
int eating_a_player(int pl_fd, int x, int y, int score){
    struct player *p = head;

    int score_to_add = 0;

    while(p){
        if (pl_fd != p->playerfd && p->x == x && p->y == y && p->dead == 0){
            // either p eats the player,
            //  nothing happens(score is equal);
            //  or the player eats p

            if (score > p->score){
                // inform p that its dead and add pl_fd->score the p score
                char *mBuff = malloc(sizeof(char) * 2);
                mBuff[0] = '8';
                mBuff[1] = '\0';

                if (send(p->playerfd, mBuff, 2, 0) != 2) {
                    Die("eating_a_player mismatch:");
                }

                p->dead = 1;

                free(mBuff);

                score_to_add += p->score;
            } else if (score < p->score) {
                // inform pl_fd from set_new_coordinates that its dead and add p->score the pl_fd score
                char *mBuff = malloc(sizeof(char) * 2);
                mBuff[0] = '8';
                mBuff[1] = '\0';

                if (send(pl_fd, mBuff, 2, 0) != 2) {
                    Die("eating_a_player mismatch:");
                }

                return -1;

                free(mBuff);

                p->score += score;
            }
        }

        p = p->next;
    }

    return score_to_add;
}

void set_new_coordinates(int pl_fd, int x, int y){
    struct player *p = head;

    int new_value;
    while(p){
        if (p->playerfd == pl_fd){
            if (x == -1){
                new_value = p->x - 1;

                if (map[p->y][new_value] == ' ') {
                    --p->x;
                }
            } else if (x == 1){
                new_value = p->x + 1;

                if (map[p->y][new_value] == ' ') {
                    ++p->x;
                }
            }

            if (y == -1){
                new_value = p->y - 1;

                if (map[new_value][p->x] == ' ') {
                    --p->y;
                }
            } else if (y == 1){
                new_value = p->y + 1;

                if (map[new_value][p->x] == ' ') {
                    ++p->y;
                }
            }

            if (check_for_found_food(p->x, p->y) == 1){
                p->score++;
            }

            int score_to_add = eating_a_player(pl_fd, p->x, p->y, p->score);

            if (score_to_add == -1){
                // this player has been eaten
                p->dead = 1;
            } else {
                p->score += score_to_add;
            }
            
            printf("new cords: %d %d\n", p->x, p->y);

            break;
        }

        p = p->next;
    }
}

// status = connected 0 or not 1
void refresh_player_status(int plfd){
    struct player *p = head;

    while(p){
        printf("plfd %d, client %d\n", plfd, p->playerfd);
        if (plfd == p->playerfd){
            printf("client %d status changes\n", plfd);
            p->status = 1;
            break;
        }

        p = p->next;
    }
}

void move_listener(struct pollfd * fds, int plc) {
    int rc;

    // Polling usage taken from https://www.ibm.com/docs/en/i/7.4?topic=designs-using-poll-instead-select
    rc = poll(fds, plc, 95); 
    printf("after poll call playerCount %d \n", plc);

    /***********************************************************/
    /* Check to see if the poll call failed.                   */
    /***********************************************************/
    if (rc < 0)
    {
      //printf("  poll() failed\n");
      //break;
    }

    /***********************************************************/
    /* Check to see if the time out expired.          */
    /***********************************************************/
    if (rc == 0)
    {
      //printf("  poll() timed out.\n");
      //break;
    }

    int k = 0;
    for (;k < plc; k++){
        printf("during poll for loop playerCount %d \n", plc);

        char *recvBuff = malloc(sizeof(char) * 3);
        if (fds[k].revents == POLLIN) {
            rc = recv(fds[k].fd, recvBuff, 3, MSG_DONTWAIT); // sizeof(buffer)
            if (rc < 0)
            {
                //printf("\n\n reading failed\n\n");
            } else {
                printf("\n\nrecv from client %d info: %s %c\n\n", fds[k].fd, recvBuff, recvBuff[1]);
                int x_out = 0;
                int y_out = 0;

                switch (recvBuff[1]){
                  case 'U':
                    y_out = -1;
                    break;
                  case 'R':
                    x_out = 1;
                    break;
                  case 'D':
                    y_out = 1;
                    break;
                  case 'L':
                    x_out = -1;
                    break;
                  case 'Q':
                    printf("Closing client %d\n", fds[k].fd);
                    refresh_player_status(fds[k].fd);
                    closefd = fds[k].fd;
                    break;
                  default:
                      printf("unrecognized movement\n");
                }

                set_new_coordinates(fds[k].fd, x_out, y_out);
            }
        }

        free(recvBuff);
    }
    
}

void game_end(){
    struct player *p = head;

    int new_value;

    char *mBuff = malloc(sizeof(char) * 5);

    while(p){
        // inform the losers that are still logged on that the game ended and close the socket
        if (p->status == 0) {
            mBuff[0] = '9';
            mBuff[1] = (p->score)/10 + 48;
            mBuff[2] = ((p->score) % 10) + 48;

            mBuff[3] = 0 + 48;

            mBuff[4] = '\0';

            printf("sending to the loser %s \n", mBuff);

            if (send(p->playerfd, mBuff, 5, 0) != 5) {
                Die("game_end mismatch:");
            }
        }
        close(p->playerfd);
        p = p->next;
    }

    close(serversock);
    
    printf("Game end!");

    free(mBuff);

    exit(0);
}

void game_update(){

    for(;;){
        char *mBuff = malloc(sizeof(char) * 255);
        int i = 2;

        mBuff[0] = '7';
        mBuff[1] = 48+playerCount;

        struct player *p = head;

        if (closefd != 0){
            while(p) {
                if (p->playerfd == closefd){
                    p->dead = 2;
                }
                p = p->next;
            }
            int shres = shutdown(closefd, SHUT_RDWR);
            if (shres != 0){
                printf("shutdown problem\n");
                fprintf(stderr, "Value of errno: %d\n", errno);
                perror("Error printed by perror");
            }

            shres = close(closefd);
            if (shres != 0){
                printf("close Problem\n");
                fprintf(stderr, "Value of errno: %d\n", errno);
                perror("Error printed by perror");
            }

            closefd = 0;
        }

        struct pollfd * fds = malloc(sizeof(struct pollfd) * playerCount);
        int j = 0;
        while(p){
            if (fcntl(p->playerfd, F_GETFD) != -1 && p->status == 0){ // p->dead == 0
                printf("Adding client %d\n", p->playerfd);
                fds[j].fd = p->playerfd;
                fds[j].events = POLLIN; 
                j++;
            }

            p = p->next;
        }

        move_listener(fds, j);

        usleep(100000);

        p = head;

        while(p){
            //if (p->dead == 0){
            // Add symbol
            mBuff[i] = p->symbol;
            i++;

            // Add score
            mBuff[i] = (p->score)/10 + 48;
            i++;
            mBuff[i] = ((p->score) % 10) + 48;
            i++;

            // Add coordinates
            mBuff[i] = (p->x)/10 + 48;
            i++;
            mBuff[i] = ((p->x) % 10) + 48;
            i++;

            mBuff[i] = (p->y)/10 + 48;
            i++;
            mBuff[i] = ((p->y) % 10) + 48;
            i++;
            // }

            mBuff[i] = p->dead + 48;
            i++;

            if (p) {
                mBuff[i] = ':';
                i++;
            }  
            //}

            p = p->next;
        }

        mBuff[i] = '-';
        i++;

        int food_it = 0;
        for (; food_it < food_count; food_it++) {
            if (food_positions[food_it][2] == 0) {
                mBuff[i] = (food_positions[food_it][0])/10 + 48;
                i++;
                mBuff[i] = ((food_positions[food_it][0]) % 10) + 48;
                i++;

                mBuff[i] = (food_positions[food_it][1])/10 + 48;
                i++;
                mBuff[i] = ((food_positions[food_it][1]) % 10) + 48;
                i++;

                mBuff[i] = ':';
                i++;
            }
        }

        mBuff[i] = 'e';

        i++;

        mBuff[i] = '\0';

        printf("usr info: %s\n", mBuff);

        unsigned int plLen = strlen(mBuff);

        p = head;
        // Send the new info to players
        //   any player who is dead or alive

        while(p){
            int fdch = fcntl(p->playerfd, F_GETFD);
            printf("fdch = %d\n", fdch);
            if (fdch != -1 && p->status == 0 && p->dead < 2) { //p->dead == 0
                printf("update sent to player %d\n", p->playerfd);
                if (send(p->playerfd, mBuff, plLen, 0) != plLen) {
                    Die("Player update fail:");
                }
            }
            

            p = p->next;
        }

        if (get_players_alive() == 1) {
            check_if_a_player_has_won();
        }

        free(mBuff);
        mBuff = NULL;

        free(fds);
        fds = NULL;
    }
}

int main(int argc, char *argv[]) {
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

            tv.tv_sec = 2;
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