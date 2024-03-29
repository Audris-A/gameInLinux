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
#include <signal.h>

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
int my_score = 0;
int sock;
int left_game = 0;
int game_ended;
pthread_t clientThread;

struct scoreboard {
  char player_symbol;
  int score;
  int dead;

  struct scoreboard *next;
};

struct scoreboard *head_score = NULL;

int got_init_player_count = 0;
int init_player_count = 0;

struct player {
  char name[17];

  struct player *next;
};

struct player *head = NULL;

//Izveidot jaunu mezglu player sarakstā
void insert_score(char *pl_symbol, int score, int deadOrAlive) 
{   
    //Izveido pašu elementu
    struct scoreboard *elem = (struct scoreboard*) malloc(sizeof(struct scoreboard));

    elem->player_symbol = pl_symbol;
    elem->score = score;

    elem->dead = deadOrAlive;

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
void Die(char *mess) { 
  perror(mess); 
  game_ended = 1;
  pthread_exit(NULL);
  //exit(1); 
}

void exit_peacfullly(char *mess){
  printf(mess);
  game_ended = 1;
  pthread_exit(NULL);
  //exit(0);
};

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
  /*strcpy(new_node->symbol, symbol);*/

  /* link the old list off the new node */
  new_node->next = *head;

  /* move the head to point to the new node */
  *head = new_node;
}

void HandleMessages(int sock) {
  char *mBuff = malloc(sizeof(char) * 255);
  int received = -1;
  int threadJoinStatus;

  int joinRes = 0;

  /* Receive message */
  if ((received = recv(sock, mBuff, BUFFSIZE, 0)) < 0) {
  Die("Failed to receive initial bytes from server");
  } else {
  if (left_game == 0){
    switch (mBuff[0]){
      case '2':
          print_lobby_info(mBuff);
          break;
      case '3':
          game_ended = 1;
          Die("Game in progress");
          break;
      case '4':
          game_ended = 1;
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
          lost = 1;
          break;
      case '9':
          // Game ended, probably lost.
          game_ended = 1;
          system("clear");
          int k = 0;
          for (; k < map_height; k++){
            free(map[k]);
          }

          game_end(mBuff);
          free(mBuff);
          free(map);
          delete_score_board(&head_score);
          deletePlayers(&head);

          usleep(50000);
          pthread_cancel(clientThread);
          usleep(50000);
          //joinRes = pthread_join(clientThread, threadJoinStatus);

          // printf("join res = %d %d\n", joinRes, threadJoinStatus);

          // if (PTHREAD_CANCELED == threadJoinStatus){
          //   printf("yup, its cancelled!\n");
          // }

          exit_peacfullly("\n");
    }
    } else {
      game_ended = 1;
      system("clear");

      //usleep(150000);
      printf("\nYou left the game! :( Your score: %d\n\n", my_score);

      free(mBuff);
      int k = 0;
      for (; k < map_height; k++){
        free(map[k]);
      }
      free(map);
      delete_score_board(&head_score);
      deletePlayers(&head);

      usleep(50000);
      pthread_cancel(clientThread);
      usleep(50000);
      //joinRes = pthread_join(clientThread, threadJoinStatus);

      // printf("join res = %d %d\n", joinRes, threadJoinStatus);

      // if (PTHREAD_CANCELED == threadJoinStatus){
      //   printf("yup, its cancelled!\n");
      // }

      exit_peacfullly("");
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

/*old list clean*/
  deletePlayers(&head);
/*terminala iztirisana*/
  system("clear");
  
  printf("Lobby.\n\n");

  if (got_init_player_count == 0){
    init_player_count = mBuff[1] - 48;

    got_init_player_count = 1;
  }

  playerCount = mBuff[1];

  printf("Player count: %c\n", playerCount);

  while(mBuff[i] != '}') {
    int k = get_name(mBuff, playerName, i);
    i = i + k;

    pushPlayers(&head, playerName);

    if (mBuff[i] == '}') {
      break;
    } else {i++;}
  }

  printPlayers(&head);

  free(playerName);
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

  map_width = get_number_map_rows(mBuff, i, 0);
  i+=3;
  map_height = get_number_map_rows(mBuff, i, 1);
  i+=2;

  /*atmina prieks kartes rindam*/
  map = (char**)malloc(sizeof(char*)*map_height);

  free(playerName);
}

void add_map_row(char* mBuff){
  int i=1;
  int n=0;
  int row_num;
  //char* row = (char*)malloc(sizeof(char)*map_width + 1);

  row_num = get_number_map_rows(mBuff, i, 0);
  i+=3;

  map[row_num-1] = (char*)malloc(sizeof(char)*map_width + 1);

  while (n < map_width){
    //row[n] = mBuff[i];
    map[row_num-1][n] = mBuff[i];
    i++;
    n++;
  }
  map[row_num-1][n] = 0;

  //row[n] = 0;

  //map[row_num-1] = row;

  printf("%s", map[row_num-1]);

  //free(row);
}

void game_update(char* mBuff){
  int i=1;
  int n=0;
  int playerCount;
  int playersInfo[8][4]; /*1-x, 2-y, 3-points, alive or dead*/
  int foodCount;
  int foodCoordinates[16][2]; /*1-x, 2-y*/
  int scores[8];
  char symbols[8];// = {'A','B','C','D','E','F','G','H'};
  //char mapToPrint[map_height][map_width];
  char** mapToPrint = (char**)malloc(sizeof(char*)*map_height);

  if (game_status == 0){
    game_status = 1;
  }

  playerCount = mBuff[i]-48;

  i++;
  int player_count_sync = 0; // fixes possible misinformation when eating a player

  // Score acquiring
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

    playersInfo[n][3] = mBuff[i] - 48;
    i+=1;

    scores[n] = playersInfo[n][0];
    
    i++;
    n++;

    player_count_sync++;
  }

  playerCount = player_count_sync;

  i++;
  n = 0;
  // // Food acquiring
  // while (mBuff[i] != 'e'){
  //   foodCoordinates[n][0] = get_number(mBuff, i, 0);
  //   i+=2;
  //   foodCoordinates[n][1] = get_number(mBuff, i, 0);
  //   i+=2;

  //   i++;
  //   n++;
  // }

  int food_i = i;

  //foodCount = n;

  i = 0;
  // Copy default map to map to show
  for (;i < map_height;i++){
    mapToPrint[i] = (char*)malloc(sizeof(char)*(map_width + 2));
    memset(mapToPrint[i], 0, map_width + 2);
    strcpy(mapToPrint[i], map[i]);
  }

  i=0;
  // Insert players on the map
  for (;i < playerCount;i++){
    if (playersInfo[i][3] == 0){
      mapToPrint[playersInfo[i][2]][playersInfo[i][1]] = symbols[i];
    }
  }

  i=0;
  // Insert food on the map
  // Food acquiring

  while (mBuff[food_i] != 'e'){
    int first = get_number(mBuff, food_i, 0);
    food_i += 2;
    int second = get_number(mBuff, food_i, 0);
    food_i += 2;

    mapToPrint[second][first] = '@';
    // foodCoordinates[n][0] = get_number(mBuff, i, 0);
    // i+=2;
    // foodCoordinates[n][1] = get_number(mBuff, i, 0);
    // i+=2;

    food_i++;
    n++;
  }

  // for (i;i < foodCount;i++){
  //   mapToPrint[foodCoordinates[i][1]][foodCoordinates[i][0]] = '@';
  // }

  i=0;
  system("clear");

  delete_score_board(&head_score);

  // Sort the player list
  int previous_k = 0;
  for (;i < playerCount; i++){
      int k = 0;
      char sym_to_insert;
      int score_to_insert = 100;
      int deadOrAlive = 0;
      for (;k < playerCount; k++){
        if (scores[k] < score_to_insert){
          score_to_insert = scores[k];
          sym_to_insert = symbols[k];
          previous_k = k;
          deadOrAlive = playersInfo[k][3];
        }
      }

      scores[previous_k] = 1000;

      insert_score(sym_to_insert, score_to_insert, deadOrAlive);
  }

  struct scoreboard *p = head_score;

  if (lost == 1) {
    printf("You lost! :( Your score: %d\n", my_score);
  }

  printf("Score table:\n");
  i = 0;
  while(p){
    if (symbols[playerCount - init_player_count] == p->player_symbol){
      printf(" You -> Player %c: %d\n", p->player_symbol, p->score);
      my_score = p->score;
    } else if (p->dead == 1) {
      printf("Dead -> Player %c: %d\n", p->player_symbol, p->score);
    } else if (p->dead == 2) {
      printf("Quit -> Player %c: %d\n", p->player_symbol, p->score);
    } else {
      printf("        Player %c: %d\n", p->player_symbol, p->score);
    }

    i++;

    p = p->next;
  }

  i = 0;
  for (; i < map_height; i++){
    printf("%s", mapToPrint[i]);
  }

  i = 0;
  for (; i < map_height; i++){
    free(mapToPrint[i]);
  }

  free(mapToPrint);
}

void game_end(char* mBuff){
  int playerCount;
  int i=1;
  int n=0;
  char playerName[20];
  char results[8][20];
  int chars[8];

  //system("clear");

  my_score = ((mBuff[1] - 48) * 10) + (mBuff[2] - 48);

  if (mBuff[3] == '1') {
    // won the game
    printf("\nCongratulations, you won!\n Your score: %d\n\n", my_score);
  } else {
    printf("\nYou lost! :(\n Your score: %d\n\n", my_score);
  }
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
    case 'l':
      dest[1] = 'Q';
      left_game = 1;
      //exit_peacfullly("Left game");
      break;
    default:
      return;
  }

  dest[2] = '\0';

  send(sock, dest, 3, 0);
}

void control_char(int sock){
  char c;

  /*button listener (works only when game in progress)*/
  for (;;){
    if (game_ended == 1) {
      pthread_exit(NULL);
    }

    c = getch();

    if(game_status == 1 && lost == 0){

      if (c == 'w' || c == 'a' || c == 's' || c == 'd' || c == 'l'){
        move(sock, c);
      }

      c = '0';
    } else if (game_status == 1 && lost == 1) {
      if (c == 'l'){
        move(sock, c);
      }

      c = '0';
    }
  }
}

int main(int argc, char *argv[]) {
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

  system("clear");
  
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

  free(username);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&clientThread, &attr, control_char, sock);

  for(;;){  
    /*server message handler*/
    HandleMessages(sock);
  }
}
