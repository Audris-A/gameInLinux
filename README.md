# Maze game
  - Authors: Audris Arzovs (50%) un Viktors Vradijs (50%)
  
  - server:
  - gcc server.c -o server -lpthread
  - Server launch format: {server} {port path to map} (./server 4098 map.txt);

  - client:
  - gcc client.c -o client
  - Client launch format: {client} {server_ip} {port} (./client 192.76.8.12 4098).