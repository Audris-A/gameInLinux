#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define BUFFSIZE 255

void print_lobby_info(char*);
void game_start(char*);
void add_map_row(char*);
void game_update(char*);
void game_end(char*);

char** map;
int map_width;
int map_height;

void Die(char *mess) { perror(mess); exit(1); }

struct player {
	char name[17];
	int score;
	/*char symbol[2];*/

	struct player *next;
};

struct player *head = NULL;

void deletePlayers(struct player** head)
{
   /* deref head_ref to get the real head */
   struct player* current = *head;
   
   struct player* next;

   while (current != NULL)
   {
   	next = current->next;
   	free(current);
   	current = next;
   }

   /* deref head_ref to affect the real head back
  	in the caller. */
   *head = NULL;
}

void printPlayers(struct player** head)
{
   /* deref head_ref to get the real head */
   struct player* current = *head;
   struct player* next;

   while (current != NULL)
   {
   	//printf("%s - %s\n", current->name, current->symbol);
   	printf("%s\n", current->name);
   	current = current->next;
   }
}

void pushPlayers(struct player** head, char* name/*, char* symbol*/)
{
	/* allocate node */
	struct player* new_node = (struct player*) malloc(sizeof(struct player));

	/* put in the data  */
	strcpy(new_node->name, name);
	// strcpy(new_node->symbol, symbol);

	/* link the old list off the new node */
	new_node->next = *head;

	/* move the head to point to the new node */
	*head = new_node;
}

void HandleMessages(int sock) {
  char *mBuff = malloc(sizeof(char) * 255);
  int received = -1;
  /* Receive message */
  if ((received = recv(sock, mBuff, BUFFSIZE, 0)) < 0) {
	Die("Failed to receive initial bytes from server");
  } else {
	switch (mBuff[0]){
    	case '2':
        	print_lobby_info(mBuff);
        	break;
    	case '3':
        	Die("Game in progress");
        	break;
    	case '4':
        	Die("Name is already taken");
        	break;
    	case '5':
        	game_start(mBuff);
        	break;
    	case '6':
        	add_map_row(mBuff);
        	break;
    	case '7':
        	game_update(mBuff);
        	break;
    	case '8':
        	Die("Game over");
        	break;
    	case '9':
        	game_end(mBuff);
        	break;
    	default:
    	//Die(mBuff);
        	printf("%s\n", mBuff);
	}
  }

  free(mBuff);
  //close(sock);
}

int get_name(char *fullStr, char *playerName/*, char *symbol*/, int start){
  int i=0;
 
  do {
	playerName[i] = fullStr[start];
	i++;
	start++;
  } while (fullStr[start] != ',' && fullStr[start] != '}');
// start++;
// i++;
 playerName[i]=0;
// symbol[0]=fullStr[start];
// symbol[1]=0;
  return i;
}

//lobby info apstrade
void print_lobby_info(char *mBuff){
	char playerCount;
	int i = 3;                                                                                                                 	 
	int n = 0;
	char *playerName = malloc(sizeof(char) * 17);
	/*char *symbol = malloc(sizeof(char) * 2);*/

//veca saraksta iztirisana
	deletePlayers(&head);
//terminala iztirisana
	system("clear");

 	playerCount = mBuff[1];

 	printf("Player count: %c\n", playerCount);

	while(mBuff[i] != '}') {
    	i = i + get_name(mBuff, playerName/*, symbol*/, i);

        	pushPlayers(&head, playerName/*, symbol*/);

        	if(mBuff[i] == '}'){
          	break;
        	} else {i++;}

	}

	printPlayers(&head);

}

int get_number(char* mBuff, int start, int last){
	int num;
	num = (mBuff[start]-48)*100;
	start++;
	num += (mBuff[start]-48)*10;
	start++;
	num += mBuff[start]-48;
	if (last == 1){
    	start++;
	}

	return num;
}

void game_start(char* mBuff){
	int i = 1;
	//char symbols[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
	int playerCount;
	char *playerName = malloc(sizeof(char) * 17);

	playerCount = mBuff[i];

	deletePlayers(&head);

	i += 2;

	while(mBuff[i] != '}') {
    	i = i + get_name(mBuff, playerName/*, symbol*/, i);

        	pushPlayers(&head, playerName/*, symbol*/);

        	if(mBuff[i] == '}'){
          	i++;
          	break;
        	} else {i++;}

	}

	map_width = get_number(mBuff, i, 0);
	i+=3;
	map_height = get_number(mBuff, i, 1);
	i+=2;

	// atmina prieks kartes rindam
	map = (char**)malloc(sizeof(char*)*map_height);

}

void add_map_row(char* mBuff){
	int i=1;
	int n=0;
	int row_num;
	char* row = (char*)malloc(sizeof(char)*map_width);

	row_num = get_number(mBuff, i, 0);
	i+=3;

	while (n < map_width){
  	row[n] = mBuff[i];
  	i++;
  	n++;
	}

	row[n] = 0;

	map[row_num-1] = row;
}

void game_update(char* mBuff){
  int i=1;
  int n=0;
  int playerCount;
  int playersInfo[8][3]; //1-x, 2-y, 3-points
  int foodCount;
  int foodCoordinates[5][2]; //1-x, 2-y
  char symbols[8] = {'A','B','C','D','E','F','G','H'};
  char** mapToPrint = (char**)malloc(sizeof(char*)*map_width);

  playerCount = mBuff[i]-48;

  i += 2;

  while (n < playerCount){
	playersInfo[n][0] = get_number(mBuff, i, 0);
	i+=3;
	playersInfo[n][1] = get_number(mBuff, i, 0);
	i+=3;
	playersInfo[n][2] = get_number(mBuff, i, 0);
	i+=3;
	if(n != playerCount-1){
  	i++;
	}
	n++;
  }

  foodCount = mBuff[i]-48;

  i += 2;
  n = 0;
  while (n < foodCount){
	foodCoordinates[n][0] = get_number(mBuff, i, 0);
	i+=3;
	foodCoordinates[n][1] = get_number(mBuff, i, 0);
	i+=3;
	if (n != foodCount-1){
  	i++;
	}
	n++;
  }

  i = 0;
  for (i;i < map_height;i++){
  	mapToPrint[i] = (char*)malloc(sizeof(char)*map_width);
  	strcpy(map[i],mapToPrint[i]);
  }
  i=0;
  for (i;i < playerCount;i++){
  	mapToPrint[playersInfo[i][1]][playersInfo[i][0]] = symbols[i];
  }
  i=0;
  for (i;i < foodCount;i++){
  	mapToPrint[foodCoordinates[i][1]][foodCoordinates[i][0]] = '@';
  }
  i=0;
  system("clear");
  for (i; i < map_height; i++){
  	printf("%s\n", mapToPrint[i]);
  }



}

void game_end(char* mBuff){
  int playerCount;
  int i=1;
  int n=0;
  char playerName[20];
  char results[8][20];
  int chars[8];

playerCount = mBuff[i]-48;

i++;

while(mBuff[i] != '}') {
    	chars[n] = get_name(mBuff, playerName, i);

    	strcpy(playerName,results[n]);
    	n++;
    	if(mBuff[i] == '}'){
        	break;
    	} else {i++;}

	}

	system("clear");

	i = 0;
	for (i; i < playerCount; i++){
  	n=0;
  	while (n < chars[i]-3){
    	printf("%c", results[i][n]);
    	n++;
  	}

  	printf("\t");

  	while (n < chars[i]){
    	printf("%c", results[i][n]);
    	n++;
  	}

  	Die("\n");

	}

}

int main(int argc, char *argv[]) {
	int sock;
	struct sockaddr_in echoserver;
	//char buffer[BUFFSIZE];
	char *username = malloc(sizeof(char) * 17);
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

	for (userLen; userLen < 17; userLen++){
    	username[userLen] = 0;
	}
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





