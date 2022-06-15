#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <termios.h>
#include <pthread.h>
#include <time.h>
#include <poll.h>

#define BUFFSIZE 255

void print_lobby_info(char*);
void game_start(char*);
void add_map_row(char*);
void game_update(char*);
void game_end(char*);

char** map; /*map array*/
int map_width;
int map_height;
int game_status; /*0 - waiting for game, 1 - game in progress*/
int lost;

struct scoreboard {
  char player_symbol;
  int score;

  struct scoreboard *next;
};

struct scoreboard *head_score = NULL;

int got_init_player_count = 0;
int init_player_count = 0;

//Izveidot jaunu mezglu player sarakstā
void insert_score(char *pl_symbol, int score) 
{   
    //Izveido pašu elementu
    struct scoreboard *elem = (struct scoreboard*) malloc(sizeof(struct scoreboard));

    elem->player_symbol = pl_symbol;
    elem->score = score;

    //Izveido norādi uz iepriekšējo pirmo elementu
    elem->next = head_score;

    //Norāda, ka jaunizveidotais elements ir pirmais
    head_score = elem;

    //printf("inserting: %c %d\n", pl_symbol, score);
};

void delete_score_board(struct scoreboard** head)
{
   /* deref head_ref to get the real head */
   struct scoreboard* current = *head;
   
   struct scoreboard* next;

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

/*getch function from conio.h (not tested)*/
int getch(){
  struct termios oldattr, newattr;
  int ch;
  tcgetattr(STDIN_FILENO, &oldattr);
  newattr = oldattr;
  newattr.c_lflag &= ~( ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
  return ch;
}

/*exit with error*/
void Die(char *mess) { perror(mess); exit(1); }

void exit_peacfullly(char *mess){
  printf(mess);
  exit(0);
};

struct player {
  char name[17];
  //int score;
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
   /* deref head to get the real head */
   struct player* current = *head;

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

  //printf("newplayer\n");

  /* put in the data  */
  strcpy(new_node->name, name);
  /*strcpy(new_node->symbol, symbol);*/

  /* link the old list off the new node */
  new_node->next = *head;

  /* move the head to point to the new node */
  *head = new_node;
}

void HandleMessages(int sock) {
  char *mBuff = malloc(sizeof(char) * 255);
  int received = -1;
  /* Receive message */
  if ((received = recv(sock, mBuff, BUFFSIZE, 0)) < 0) { // game_status == 0 && || (game_status == 1 && (received = recv(sock, mBuff, 11, 0)) < 0)
  Die("Failed to receive initial bytes from server");
  } else {
  switch (mBuff[0]){
      case '2':
          print_lobby_info(mBuff);
          printf("lobby\n");
          break;
      case '3':
          printf("game\n");
          Die("Game in progress");
          break;
      case '4':
          Die("Name is already taken");
          break;
      case '5':
          printf("start\n");
          game_start(mBuff);
          break;
      case '6':
          //printf("%s\n", mBuff);
          add_map_row(mBuff);
          break;
      case '7':
          /*system("clear");
          if (game_status == 0){
            game_status = 1;
          }

          printf("recv: %s\n", mBuff);*/

          /*int buffLen = 2+5*(mBuff[1]-48)-1; // other chars are junk
          char *updatebuff = malloc(sizeof(char) * buffLen + 1);

          strncpy(updatebuff, mBuff, buffLen);
          updatebuff[buffLen] = '\0';

          printf("recv: %s\n", updatebuff);


          free(updatebuff);*/

          //printf("7\n\n");
          game_update(mBuff);
          break;
      case '8':
          printf("%s\n", mBuff);
          //exit_peacfullly("You lost!\n");
          lost = 1;
          //Die("Game over");
          break;
      case '9':
          game_end(mBuff);
          exit_peacfullly("You win! Your score: \n");
          //Die("\n");
          break;
      //default:
          //printf("DEFAULT %s\n", mBuff);
  }
  }

  free(mBuff);
}

int get_name(char *fullStr, char *playerName/*, char *symbol*/, int start){
  int i=0;
 
  while (i<16 && fullStr[start + i] != ':' && fullStr[start + i] != '}') {
    //printf("%c\n", fullStr[start + i]);
    playerName[i] = fullStr[start + i];
    i++;
  }
  
  playerName[i]=0;
  return i;
}

/*lobby info*/
void print_lobby_info(char *mBuff){
  char playerCount;
  int i = 3;                                                                                                                   
  int n = 0;
  char *playerName = malloc(sizeof(char) * 17);
  /*char *symbol = malloc(sizeof(char) * 2);*/

/*old list clean*/
  deletePlayers(&head);
/*terminala iztirisana*/
  system("clear");

  if (got_init_player_count == 0){
    init_player_count = mBuff[1] - 48;

    got_init_player_count = 1;

    printf("init_player_count = %d\n", init_player_count);
  }

  playerCount = mBuff[1];

  printf("Player count: %c\n", playerCount);
  //printf("%s\n\n", mBuff);

  while(mBuff[i] != '}') {
    int k = get_name(mBuff, playerName, i);
    i = i + k;

    pushPlayers(&head, playerName);

    //printf("k = %d, s = %s\n", k, playerName);
    //printf("strbuf = %c\n", mBuff[i]);

    if (mBuff[i] == '}') {
      break;
    } else {i++;}
  }

  printPlayers(&head);
}

int get_number_map_rows(char* mBuff, int start, int last){
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

int get_number(char* mBuff, int start, int last){
  int num;
  //num = (mBuff[start]-48)*100;
  //start++;
  num = (mBuff[start]-48)*10;
  start++;
  num += mBuff[start]-48;
  if (last == 1){
      start++;
  }

  return num;
}

void game_start(char* mBuff){
  int i = 1;
  int playerCount;
  char *playerName = malloc(sizeof(char) * 17);

  playerCount = mBuff[i];

  deletePlayers(&head);

  i += 2;

  while(mBuff[i] != '}') {
    i = i + get_name(mBuff, playerName, i);

    pushPlayers(&head, playerName);

    if(mBuff[i] == '}'){
      i++;
      break;
    } else {i++;}
  }

  //printf("meta = %s\n", mBuff);

  map_width = get_number_map_rows(mBuff, i, 0);
  i+=3;
  map_height = get_number_map_rows(mBuff, i, 1);
  i+=2;

  /*atmina prieks kartes rindam*/
  map = (char**)malloc(sizeof(char*)*map_height);
}

void add_map_row(char* mBuff){
  int i=1;
  int n=0;
  int row_num;
  char* row = (char*)malloc(sizeof(char)*map_width);

  row_num = get_number_map_rows(mBuff, i, 0);
  i+=3;

  while (n < map_width){
    row[n] = mBuff[i];
    i++;
    n++;
  }

  row[n] = 0;

  map[row_num-1] = row;

  printf("%s", map[row_num-1]);
}

void game_update(char* mBuff){
  int i=1;
  int n=0;
  int playerCount;
  int playersInfo[8][3]; /*1-x, 2-y, 3-points*/
  int foodCount;
  int foodCoordinates[16][2]; /*1-x, 2-y*/
  int scores[8];
  char symbols[8];// = {'A','B','C','D','E','F','G','H'};
  char** mapToPrint = (char**)malloc(sizeof(char*)*map_width);

  int myScore = 0;

  //printf("\n\nGAME UPDATE!!!\n\n");

  if (game_status == 0){
    game_status = 1;
  }

  playerCount = mBuff[i]-48;

  i++;
  int player_count_sync = 0; // fixes possible misinformation when eating a player
  while (n < playerCount){
    if (mBuff[i] == '-'){
      break;
    }

    symbols[n] = mBuff[i];
    i++;

    playersInfo[n][0] = get_number(mBuff, i, 0);
    i+=2;
    playersInfo[n][1] = get_number(mBuff, i, 0);
    i+=2;
    playersInfo[n][2] = get_number(mBuff, i, 0);
    i+=2;

    scores[n] = playersInfo[n][0];
    
    // if (n == (init_player_count - 1)){
    //   myScore = playersInfo[n][0];
    // }

    //if(n != playerCount-1){
    i++;
    //}
    n++;

    player_count_sync++;
  }

  playerCount = player_count_sync;

  //foodCount = mBuff[i]-48;

  //printf("alive0\n");

  i++;
  n = 0;
  while (mBuff[i] != 'e'){
    foodCoordinates[n][0] = get_number(mBuff, i, 0);
    i+=2;
    foodCoordinates[n][1] = get_number(mBuff, i, 0);
    i+=2;
    //if (n != foodCount-1){
    i++;
    //}
    n++;

    //printf("food cnt = %d\n", n);
  }

  //printf("alive1\n");

  //printf("food cnt = %d\n", n);
  foodCount = n;

  i = 0;
  for (i;i < map_height;i++){
    mapToPrint[i] = (char*)malloc(sizeof(char)*map_width);
    strcpy(mapToPrint[i], map[i]);
  }

  //printf("alive2\n");

  i=0;
  for (;i < playerCount;i++){
    //printf("pii1 %d\n", playersInfo[i][1]);
    //printf("pii0 %d\n", playersInfo[i][0]);
    //printf("sym %c\n", symbols[i]);
    mapToPrint[playersInfo[i][2]][playersInfo[i][1]] = symbols[i];
  }

  //printf("alive3\n");

  i=0;
  for (i;i < foodCount;i++){
    mapToPrint[foodCoordinates[i][1]][foodCoordinates[i][0]] = '@';
  }

  i=0;
  system("clear");

  //printf("Score = %d\n", myScore);
  //printf("alive4\n");
  delete_score_board(&head_score);

  int previous_k = 0;
  for (;i < playerCount; i++){
      int k = 0;
      char sym_to_insert;
      int score_to_insert = 100;
      for (;k < playerCount; k++){
        if (scores[k] < score_to_insert){
          score_to_insert = scores[k];
          sym_to_insert = symbols[k];
          previous_k = k;
        }
      }

      scores[previous_k] = 1000;

      insert_score(sym_to_insert, score_to_insert);
  }

  struct scoreboard *p = head_score;

  if (lost == 1) {
    printf("You lost!\n");
  }

  printf("Score table:\n");
  while(p){
    printf("Player %c: %d\n", p->player_symbol, p->score);
    p = p->next;
  }

  /*for (; i < playerCount; i++){
    if (i ==  playerCount - init_player_count){
      printf("You -> Player %c: %d\n", symbols[i], scores[i]);
    } else {
      printf("       Player %c: %d\n", symbols[i], scores[i]);
    }
  }*/

  i = 0;
  for (i; i < map_height; i++){
    printf("%s", mapToPrint[i]);
  }
  printf("%s\n", mBuff);

  //printf("alive5\n");

  /*i=0;
  for  (i;i < playerCount;i++){

  }*/
}

void game_end(char* mBuff){
  int playerCount;
  int i=1;
  int n=0;
  char playerName[20];
  char results[8][20];
  int chars[8];

  /*playerCount = mBuff[i]-48;

  i++;

  while(mBuff[i] != '}') {
    chars[n] = get_name(mBuff, playerName, i);

    strcpy(playerName,results[n]);
    n++;
    if(mBuff[i] == '}'){
        break;
    } else {i++;}

  }*/

  system("clear");

  printf("You win! Your score: ");

  /*i = 0;
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

  }*/
}

void move(int sock, char c){

  char dest[3] = "1";

  switch(c){
    case 'w':
      dest[1] = 'U';
      break;
    case 'a':
      dest[1] = 'L';
      break;
    case 's':
      dest[1] = 'D';
      break;
    case 'd':
      dest[1] = 'R';
      break;
    default:
      return;
  }
  dest[2] = '\0';

  //printf("%s\n",dest);
  send(sock, dest, 3, 0);
}

void control_char(int sock){
  char c;

  /*button listener (works only when game in progress)*/
  for (;;){
    if(game_status == 1 && lost == 0){
      c = getch();

      if (c == 'w' || c == 'a' || c == 's' || c == 'd'){
        move(sock, c);
      }

      c = '0';
    }
  }
  
}

int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in echoserver;
  char *username = malloc(sizeof(char) * 17);
  char confirm[2];
  unsigned int userLen;
  int received = 0;
  char c;
  int lenght;

  game_status = 0;

  lost = 0;

  if (argc != 3) {
    fprintf(stderr, "USAGE: TCPecho <server_ip> <port>\n");
    exit(1);
  }
  /* Create the TCP socket */
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    Die("Failed to create socket");
  }

  /* Construct the server sockaddr_in structure */
  memset(&echoserver, 0, sizeof(echoserver));     /* Clear struct */
  echoserver.sin_family = AF_INET;                /* Internet/IP */
  echoserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
  echoserver.sin_port = htons(atoi(argv[2]));     /* server port */
  /* Establish connection */
  if (connect(sock,
              (struct sockaddr *) &echoserver,
              sizeof(echoserver)) < 0) {
    Die("Failed to connect with server");
  }

  printf("Enter your name:\n");
  scanf("%s", username);

  userLen = strlen(username);
  lenght = userLen+1;

  /*add 0 to message*/
  for (userLen; userLen > 0; userLen--){
      username[userLen] = username[userLen-1];
  }
  username[0] = '0';
    
  if (send(sock, username, lenght, 0) != lenght) {
    Die("Mismatch in number of sent bytes");
  }

  int threadJoinStatus = 0;
  pthread_t clientThread;
  pthread_create(&clientThread, NULL, control_char, sock);
  //pthread_join(clientThread, threadJoinStatus);

  for(;;){

    /*server message handler*/
    HandleMessages(sock);

    // /*button listener (works only when game in progress)*/
    // if(game_status == 1){
    //     c = getch();
    //     printf("After get char\n");

    //     if (c == 'w' || c == 'a' || c == 's' || c == 'd'){
    //       move(sock, c);
    //     }

    //     c = '0';
    // }
   
  }
   
}
