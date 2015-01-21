#define MAX_NAME 20
#define MAX_MESSAGE 250

#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct Client {
    char pseudo[MAX_NAME];
    struct sockaddr_in socket_addr;
} Client;

typedef struct Salon {
    char name[MAX_NAME];
    Client * clients;
} Salon;

struct Message {
    char message[MAX_MESSAGE];
    char salonCible[MAX_NAME];
} Message;

typedef struct Communication {
    char message[MAX_MESSAGE];
    int acquitted;
    struct Communication * next;
} Communication;

