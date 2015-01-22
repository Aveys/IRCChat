struct Message listeSalon();
struct Message listeCommandes();
struct Message erreurCommande();
int findClient(struct sockaddr_in adresse);
void envoyerMessageClient(struct Client client,struct Message msg);
void envoyerMessageSalon(struct Salon salon, struct Message msg);
void* realloc_s (void **ptr, size_t taille);
int startsWith(const char *pre, const char *str);
int trouverClientDansSalon(struct Salon s,struct sockaddr_in adresse);