#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// tīkla ports, pie kura konektēties
#define PORT 4097

// komandas
enum {
    CMD_ECHO,         // vienkārši sūta atpakaļ saņemtos datus
    CMD_GETHOSTNAME,  // sūta atpakaļ servera vārdu (kā ASCII rindiņu)
};

// Use type-length-value (TLV) command format
struct NetCommand_s {
    // komandas kods
    uint16_t command;
    // paketes garums, ieskaitot "command" un "length" laukus)
    uint16_t length;
    // dati (mainīga garuma)
    uint8_t data[0];
} __attribute__((packed)); // nodrošinamies pret to, ka kompilators varētu
                           // mēģināt ievietot "tukšumus" šajā struktūrā

// ērtākai dzīvei
typedef struct NetCommand_s NetCommand_t;

// maksimālais komandas paketes izmērs baitos
#define MAX_MESSAGE 1000

// Universāla datu sūtīšanas funkcija (bloķējošiem soketiem)
void sendData(int fd, char *data, unsigned length, struct sockaddr_in *remote);

#endif
