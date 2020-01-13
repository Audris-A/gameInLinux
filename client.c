#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define BUFFSIZE 255
void Die(char *mess) { perror(mess); exit(1); }

struct player {
	char name[17];
	char symbol[2];

	struct player *next;
};

struct player *head = NULL;

void deletePlayers(struct player** head)
{

   struct player* current = *head;

   struct player* next;

   while (current != NULL)
   {
   	next = current->next;
   	free(current);
   	current = next;
   }

   *head = NULL;
}

void printPlayers(struct player** head)
{
   /* deref head_ref to get the real head */
   struct player* current = *head;
   struct player* next;

   while (current != NULL)
   {
   	printf("%s - %s\n", current->name, current->symbol);
   	current = current->next;
   }
}

void pushPlayers(struct player** head, char* name, char* symbol)
{
	/* allocate node */
	struct player* new_node = (struct player*) malloc(sizeof(struct player));

	/* put data  */
	strcpy(new_node->name, name);
	strcpy(new_node->symbol, symbol);

	/* link the old list off the new node */
	new_node->next = *head;

	/* move the head to point to the new node */
	*head = new_node;
}

void print_lobby_info(char*);

void HandleMessages(int sock) {
  char *mBuff = malloc(sizeof(char) * 255);
  int received = -1;
  /* Receive message */
  if ((received = recv(sock, mBuff, BUFFSIZE, 0)) < 0) {
	Die("Failed to receive initial bytes from server");
  } else {
	//all server codes
	switch (mBuff[0]){
    	case '2':
        	print_lobby_info(mBuff);
        	break;
    	case '3':
        	break;
    	case '4':
        	break;
    	case '5':
        	break;
    	case '6':
        	break;
    	case '7':
        	break;
    	case '8':
        	break;
    	case '9':
        	break;
    	default:
        	printf("%s\n", mBuff);
	}
  }

  free(mBuff);
  //close(sock);
}

int get_name(char *fullStr, char *playerName, char *symbol, int start){
  int i=0;

  do {
	playerName[i] = fullStr[start];
	i++;
	start++;
  } while (fullStr[start] != '(');
start++;
i++;
playerName[i]=0;
symbol[0]=fullStr[start];
symbol[1]=0;
  return i+2;
}

//lobby info apstrade
void print_lobby_info(char *mBuff){
	char playerCount;
	int i = 3;
	int n = 0;
	char *playerName = malloc(sizeof(char) * 17);
	char *symbol = malloc(sizeof(char) * 2);

//veca saraksta iztirisana
	deletePlayers(&head);
//terminala iztirisana
	system("clear");

 	playerCount = mBuff[1];

 	printf("Player count: %c\n", playerCount);
//speletaju pierakstisana saraksta
	while(mBuff[i] != '}') {
    	i = i + get_name(mBuff, playerName, symbol, i);

        	pushPlayers(&head, playerName, symbol);

        	if(mBuff[i] == '}'){
          	break;
        	} else {i++;}

	}

//saraksta attelosana
	printPlayers(&head);

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
	memset(&echoserver, 0, sizeof(echoserver));   	/* Clear struct */
	echoserver.sin_family = AF_INET;              	/* Internet/IP */
	echoserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
	echoserver.sin_port = htons(atoi(argv[2]));   	/* server port */
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
  	buffer[bytes] = '\0';    	/* Assure null terminated string
  	fprintf(stdout, buffer);
	}
	fprintf(stdout, "\n");

	/* Username handler
	//recv(sock, buffer, BUFFSIZE-1, 0);
	/* Receive the word back from the server */

	//HandleMessages(sock);
	printf("Enter your name:\n");
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






