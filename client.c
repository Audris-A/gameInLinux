#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define BUFFSIZE 255
void Die(char *mess) { perror(mess); exit(1); }

void HandleMessages(int sock) {
  char *mBuff = malloc(sizeof(char) * 255);
  int received = -1;
  /* Receive message */
  if ((received = recv(sock, mBuff, BUFFSIZE, 0)) < 0) {
    Die("Failed to receive initial bytes from server");
  }


  fprintf(stdout, "%s\n", mBuff);
  free(mBuff);
  //close(sock);
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in echoserver;
    //char buffer[BUFFSIZE];
    char *username = malloc(sizeof(char) * 255);
    char confirm[2];
    unsigned int userLen;
    int received = 0;

    if (argc != 3) {
      fprintf(stderr, "USAGE: TCPecho <server_ip> <port>\n");
      exit(1);
    }
    /* Create the TCP socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      Die("Failed to create socket");
    }

    /* Construct the server sockaddr_in structure */
    memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
    echoserver.sin_family = AF_INET;                  /* Internet/IP */
    echoserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
    echoserver.sin_port = htons(atoi(argv[2]));       /* server port */
    /* Establish connection */
    if (connect(sock,
                (struct sockaddr *) &echoserver,
                sizeof(echoserver)) < 0) {
      Die("Failed to connect with server");
    }

    /* Send the word to the server 
    /*echolen = strlen(argv[2]);
    if (send(sock, argv[2], echolen, 0) != echolen) {
      Die("Mismatch in number of sent bytes");
    }
    /* Receive the word back from the server 
    fprintf(stdout, "");

    while (received < echolen) {
      int bytes = 0;
      if ((bytes = recv(sock, buffer, BUFFSIZE-1, 0)) < 1) {
        Die("Failed to receive bytes from server");
      }
      received += bytes;
      buffer[bytes] = '\0';        /* Assure null terminated string 
      fprintf(stdout, buffer);
    }
    fprintf(stdout, "\n");

    /* Username handler
    //recv(sock, buffer, BUFFSIZE-1, 0);
    /* Receive the word back from the server */

    //HandleMessages(sock);

    scanf("%s", username);

    userLen = strlen(username);
    //printf("Len - %d\n",userLen);
    //send(sock, &username, userLen, 0);
    if (send(sock, username, userLen, 0) != userLen) {
      Die("Mismatch in number of sent bytes");
    }

    //free(username);

    /* Confirmation handler */
    /* Receive the word back from the server */

    //Before game start
    for(;;){
      HandleMessages(sock);
      //Receive something that indicates the start of the game
    }
   //close(sock);
   //exit(0);
  }