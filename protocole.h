#define MAX_NAME 20
#define MAX_MESSAGE 250
#define MAX_SALONS 10
#define MAX_CLIENTS 20

#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

typedef struct Client {
    char pseudo[MAX_NAME];
    struct sockaddr_in socket_addr;
    int estVivant;
} Client;

typedef struct Salon {
    char name[MAX_NAME];
    Client clients[MAX_CLIENTS];
    int nbClient;
} Salon;

struct Message {
    char message[MAX_MESSAGE];
    char salonCible[MAX_NAME];
    time_t t;
} Message;

typedef struct Communication {
    char message[MAX_MESSAGE];
    int acquitted;
    struct Communication * suivant;
} Communication;

