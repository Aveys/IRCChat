#define MAX_NAME 20
#define MAX_CLIENT 50
#define MAX_MESSAGE 250

struct Client{
    Char nickname[MAX_NAME];
    Sockaddr_in socket_addr;
};

struct Salon{
    Char name[MAX_NAME];
    Client listeClients[MAX_CLIENT];
};

struct Message{
    Char message[MAX_MESSAGE];
    Char salonCible[MAX_NAME];
};