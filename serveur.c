#include "protocole.h"
#include "serveur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT_SERVEUR 1500

struct Client *clients=NULL;//tableau des clients connecté
int nbClients=0;
struct Salon *salons=NULL;//tableau des salons crées
int nbSalons=0;
int sd;

int main(int argc, char *argv[]) {
    //INITIALISATION DES VARIABLES GLOBALES
    clients = malloc(sizeof(Client));
    salons = malloc(sizeof(Salon));

    pthread_t thread;
    int n;

    socklen_t addr_len;
    struct sockaddr_in client_addr, server_addr;
    struct Message msg;



    // Create socket
    if ((sd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket creation");
        return 1;
    }

    // Bind it
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT_SERVEUR);
    if (bind(sd, (struct sockaddr *) &server_addr, sizeof server_addr) == -1) {
        perror("bind");
        return 1;
    }

    struct Client tmp;

    for (; ;) {
        // bloc de réception des clients
        printf("En attente d'une trame\n");
        addr_len = sizeof(client_addr);
        n = recvfrom(sd,&msg, sizeof(Message), 0, (struct sockaddr *) &client_addr, &addr_len);
        if (n == -1)
            perror("recvfrom");

        printf("Structure reçu : %s, %s\n",msg.message,msg.salonCible); //DEBUG

        if (startsWith("CONNECT",msg.message) == 1){// Vérification que le message commence par CONNECT

            printf("Demande de connection d'un nouveau client : %s\n", inet_ntoa(client_addr.sin_addr)); //DEBUG
            int indice = findClient(client_addr);
            if(indice==-1){
                printf("Ajout du client\n");
                strcpy(tmp.pseudo,"ANONYME");
                tmp.socket_addr=client_addr;
                clients[nbClients]=tmp;
                nbClients++;
                // creation de la structure de réponse à la connection
                strcpy(msg.message,"ACK_CONNECTED");
                strcpy(msg.salonCible,"");
                envoyerMessageClient(tmp, msg);
            }
            else{
                strcpy(tmp.pseudo,"ANONYME");
                tmp.socket_addr=client_addr;
                strcpy(msg.message,"ERR_IPALREADYUSED");
                strcpy(msg.salonCible,"");
                envoyerMessageClient(tmp, msg);
            }
        }
        else if (startsWith("NICK",msg.message) == 1) {
            int indice = findClient(client_addr);
            int indiceSalon=-1;
            char *nickname;
            strcpy(nickname,msg.message);
            nickname+=5;
            printf("Changement de pseudonyme de %s en %s",clients[indice].pseudo,nickname);
            for (int i = 0; i < nbSalons; ++i) {
                if( (indiceSalon = trouverClientDansSalon(salons[i], client_addr))!= -1){

                }
            }


        }
        else if (strcmp(msg.message, "JOIN") == 0) {
            fputs("USER a demande à rejoindre SALON", stdout);
        }
        else if (strcmp(msg.message, "PART") == 0) {
            fputs("USER a quitté le SALON", stdout);
        }
        else if (strcmp(msg.message, "QUIT") == 0) {
            //VERIFIER PRESENCE DANS SALON -> SUPPRIMER DES SALONS

            //supprimerClient(); SUPPRIMER DU SERVEUR
        }
        else if (strcmp(msg.message, "LIST") == 0) {
            fputs("USER a demandé la liste des salons ouvert", stdout);
            msg = listeSalon();
            //envoyerMessageClient(moi, msg);
        }
        else if (strcmp(msg.message, "HELP") == 0) {
            fputs("USER a demandé de l'aide", stdout);
            msg = listeSalon();
            //envoyerMessageClient(moi, msg);
        }
        else {
            fputs("Commande non reconnue", stdout);
            msg = erreurCommande();
            //envoyerMessageClient(moi, msg);
        }
    }
}
//renvoi l'indice du client dans le tableau
int findClient(struct sockaddr_in adresse){
    struct sockaddr_in c;
    for (int i = 0; i < nbClients; i++) {
        c=clients[i].socket_addr;
        if(strcmp(inet_ntoa(adresse.sin_addr),inet_ntoa(c.sin_addr))==0){
            printf("Client trouvé\n");
            return i;
        }
    }
    printf("Utilisateur %s introuvable\n",inet_ntoa(adresse.sin_addr));
    return -1;
}
//renvoi l'indice du client dans le salon
int trouverClientDansSalon(struct Salon s,struct sockaddr_in adresse){
    struct sockaddr_in c;
    for (int i = 0; i < s.nbClient; i++) {
        c=s.clients[i].socket_addr;
        if(strcmp(inet_ntoa(adresse.sin_addr),inet_ntoa(c.sin_addr))==0){
            printf("Client trouvé dans le salon %s\n",s.name);
            return i;
        }
    }
    printf("Utilisateur %s introuvable dans le salon %s\n",inet_ntoa(adresse.sin_addr),s.name);
    return -1;
}
struct Message listeSalon(){
    struct Message ret;
    strcpy(ret.salonCible,"NULL");
    strcpy(ret.message,"ACK_LIST");
    for(int i=0;i<nbSalons;i++){
        strcat(ret.message," ");
        strcat(ret.message,salons[nbSalons].name);
    }
    return ret;
}

struct Message listeCommandes(){
    struct Message ret;
    strcpy(ret.message,"ACK_HELP /SERVER <@IP>: Demander la connexion au serveur \n - /NICK <pseudonyme>: Changer de pseudonyme \n - /JOIN <Salon>: Rejoindre un serveur \n - /PART : Quitter le salon \n - /LIST : Lister les salons ouverts \n - /HELP : Afficher la liste des commandes possibles\n");
    strcpy(ret.salonCible,"NULL");
    return ret;
}

struct Message erreurCommande(){
    struct Message ret;
    strcpy(ret.salonCible,"NULL");
    strcpy(ret.message,"ERR_CMDUNKNOWN");
    return ret;
}

void envoyerMessageClient(struct Client client,struct Message msg){
    sendto(sd,&msg,sizeof(Message),0, (struct sockaddr *)&client.socket_addr, sizeof(client.socket_addr));
}
// Envoi un message à tous les utilisateurs d'un salon
void envoyerMessageSalon(struct Salon salon, struct Message msg){
    struct Client c;
    for (int i = 0; i < salon.nbClient; i++) {
        c=salon.clients[i];
        envoyerMessageClient(c,msg);
    }
}
int VerifierUtilisationPseudonyme(char pseudonyme[]){
    int ret=0;
    struct Client tmp;
    for (int i = 0; i < nbClients; ++i) {
        tmp = clients[i];
        if(strcmp(pseudonyme,tmp.pseudo)==0)
            ret=1;
    }
    return ret;
}
void* realloc_s (void **ptr, size_t taille){
    void *ptr_realloc = realloc(*ptr, taille);

    if (ptr_realloc != NULL)
        *ptr = ptr_realloc;
/* Même si ptr_realloc est nul, on ne vide pas la mémoire. On laisse l'initiative au programmeur. */

    return ptr_realloc;
}

int startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
            lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}
