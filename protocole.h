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
};

struct Message{
    Char message[MAX_MESSAGE];
    Char salonCible[MAX_NAME];
};

typedef enum Commande Commande;
enum Commande
{
    SERVER, NICK, JOIN, PART, QUIT, LIST, HELP
};
