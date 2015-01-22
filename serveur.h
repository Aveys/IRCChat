struct Message listeSalon();
struct Message listeCommandes();
struct Message erreurCommande();
struct Message joindreSalon(char* nomSalon, struct Client *c);
struct Message quitterSalon(char* nomSalon,struct Client c);
int findClient(struct sockaddr_in adresse);
int findSalon(char *nomSalon);
void envoyerMessageClient(struct sockaddr_in,struct Message msg);
void envoyerMessageSalon(struct Salon salon, struct Message msg);
int startsWith(const char *pre, const char *str);
int trouverClientDansSalon(struct Salon s,struct sockaddr_in adresse);
struct Client getCurrentClient(struct sockaddr_in adresse);
void printClients();
void printSalons();
void decalageClient(int pos);
void decalageSalon(int pos);
void decalageClientDansSalon(Salon s, int pos);
void getDateTime(char *t);