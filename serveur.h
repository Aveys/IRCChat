void listeSalon(struct Message *);

void listeCommandes(struct Message *);

struct Message erreurCommande();

struct Message joindreSalon(char * , struct Client *);

void quitterSalon(struct sockaddr_in ,char *);

void quitterServeur(struct sockaddr_in );

int findClient(struct sockaddr_in);

int findSalon(char *);

void envoyerMessageClient(struct sockaddr_in, struct Message );

void envoyerMessageSalon(struct Salon , struct Message );

int startsWith(const char *, const char *);

int trouverClientDansSalon(struct Salon ,struct sockaddr_in );

void printClients();

void printSalons();

void decalageClient(int );

void decalageSalon(int );

void decalageClientDansSalon(Salon , int );

void getDateTime(char *);

void changeNickname(struct sockaddr_in , struct Message *);

void connectUser(struct sockaddr_in );

void sendMessage(struct sockaddr_in , struct Message *);

void joinSalon(struct sockaddr_in , struct Message *);