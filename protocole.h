#define MAX_NAME 20
#define MAX_CLIENT 50
#define MAX_MESSAGE 250

#include <netinet/in.h>
#include <arpa/inet.h>

<<<<<<< HEAD
typedef struct Client {
    char pseudo[MAX_NAME];
    struct sockaddr_in socket_addr;
} Client;
=======
struct Salon{
    Char name[MAX_NAME];
    Client * clients;
};
>>>>>>> d021c13641e77ab2c3f2032b24f65efcd116852b

typedef struct Salon {
    char nom[MAX_NAME];
    Client clients[MAX_CLIENT];
} Salon;

typedef struct Message{
    char message[MAX_MESSAGE];
    char salonCible[MAX_NAME];
} Message;