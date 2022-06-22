## Maze game reborn
  - Author: Audris Arzovs;
  - Game has the same launch formats;
  - The game can be called as a game now, meaning, 
    - clients can join the lobby (as it was before);
    - start playing the game by moving around using W, A, S, D keys;
    - eat stuff in the maze;
    - eat other players;
    - get eaten by other players;
    - see scoreboard which holds all player scores in descending order;
    - any player can leave the game at any time with no consequences other than forfeiting the game;
      - to leave a game press L key.
    - any player that was eaten can still watch the game until it ends;
    - after a winner has been found:
      - by eating all the players or;
      - by being the last one in the game;
      - a message shows that the player is the winner and the game ends.

# Maze game
  - Initial authors: Audris Arzovs (server) un Viktors Vradijs (client) (from third course in bachelor studies):
    - Game had lobby and the start of the map sending to clients, that didn't work correctly;
    - Viktors wrote quite a lot client side pseudo-code/code for next tasks that was really helpful for the reborn version;
    - see master branch.
  - Server launch format: {server} {port path to map} (./server 4098 map.txt);
  - Client launch format: {client} {server_ip} {port} (./client 192.168.88.14 4098).

### TODOs
 - Get rid of memory leaks
   - (done) Definitely lost;
   - (done) Indirectly lost;
   - (done maybe not 100% for client) Possibly lost;
   - (done only for server) Still reachable;
   - Fix erros which Valgrind finds.