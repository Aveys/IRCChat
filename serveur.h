typedef struct Args_Thread {
    struct sockaddr_in addr;
} Args_Thread;

void *thread_client(void *arguments);
struct Message listeSalon();
struct Message listeCommandes();
struct Message erreurCommande();
struct Message joindreSalon(char* nomSalon,Client c);
struct Message quitterSalon(char* nomSalon,Client c);
void envoyerMessageClient(struct Client client,struct Message msg);
void envoyerMessageSalon(struct Salon salon, struct Message msg);
void* realloc_s (void **ptr, size_t taille);