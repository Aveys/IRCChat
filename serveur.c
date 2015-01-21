#include "protocole.h"
#include "serveur.h"

#include <stdio.h>
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
    clients = malloc(sizeof(Client));
    pthread_t thread;
    int n;
    socklen_t addr_len;
    struct sockaddr_in client_addr, server_addr;
    struct Message msg;
    struct Args_Thread args;



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

    for (; ;) {
        printf("En attente d'une connection\n");
        addr_len = sizeof(client_addr);
        n = recvfrom(sd,&msg, sizeof(Message), 0, (struct sockaddr *) &client_addr, &addr_len);
        if (n == -1)
            perror("recvfrom");
        printf("Structure reçu : %s, %s\n",msg.message,msg.salonCible);
        if (strcmp(msg.message,"CONNECT")==0) {
            printf("Connection d'un nouveau client : %s\n", inet_ntoa(client_addr.sin_addr));
            args.addr=client_addr;
            if(pthread_create(&thread, NULL, thread_client, (void *)&args) != 0){
                perror("Impossible de creer le thread");
            }
        }
    }
}

void *thread_client(void *arguments){
    int alive = 1;
    int n;
    fputs("Dans le thread\n",stdout);
    struct Args_Thread *args = arguments;

    struct Client moi;
    strcpy(moi.pseudo,"Anon");
    moi.socket_addr=args->addr;
    nbClients++;
    clients = malloc(nbClients* sizeof(Client));
    clients[nbClients]=moi;

    struct Message msg;
    strcpy(msg.message,"ACK_CONNECTED");
    strcpy(msg.salonCible,"NULL");
    sendto(sd,&msg,sizeof(Message),0, (struct sockaddr *)&moi.socket_addr, sizeof(moi.socket_addr));
    while(alive==1){
        n = recvfrom(sd,&msg, sizeof(Message), 0, (struct sockaddr *)&moi.socket_addr, sizeof(moi.socket_addr));
        if (n == -1)
            perror("recvfrom");

        if(strcmp(msg.message,"NICK")==0){

        }
        else if(strcmp(msg.message,"JOIN")==0){
            fputs("USER a demande à rejoindre SALON",stdout);
        }
        else if (strcmp(msg.message,"PART")==0){
            fputs("USER a quitté le SALON",stdout);
        }
        else if(strcmp(msg.message,"QUIT")==0){
            fputs("USER s'est déconnecté",stdout);
        }
        else if(strcmp(msg.message,"LIST")==0){
            fputs("USER a demandé la liste des salons ouvert",stdout);
            msg = listeSalon();
            envoyerMessageClient(moi, msg);
        }
        else if(strcmp(msg.message,"HELP")==0){
            fputs("USER a demandé de l'aide",stdout);
            msg = listeSalon();
            envoyerMessageClient(moi, msg);
        }
        else{
            fputs("Commande non reconnue",stdout);
            msg = erreurCommande();
            envoyerMessageClient(moi, msg);
        }
    }
}

Message listeSalon(){
    Message ret;
    strcpy(ret.salonCible,"NULL");
    strcpy(ret.message,"ACK_LIST");
    for(i=0;i<nbSalons;i++){
        strcat(ret.message," ");
        strcat(ret.message,salons.name);
    }
    return ret;
}

Message listeCommandes(){
    Message ret;
    strcpy(ret.message,"ACK_HELP /SERVER <@IP>: Demander la connexion au serveur \n - /NICK <pseudonyme>: Changer de pseudonyme \n - /JOIN <Salon>: Rejoindre un serveur \n - /PART : Quitter le salon \n - /LIST : Lister les salons ouverts \n - /HELP : Afficher la liste des commandes possibles\n");
    strcpy(ret.salonCible,"NULL");
    return ret;
}

Message erreurCommande(){
    Message ret;
    strcpy(ret.salonCible,"NULL");
    strcpy(ret.message,"ERR_CMDUNKNOWN");
    return ret;
}

void envoyerMessageClient(struct Clients client,struct Message msg){
    sendto(sd,&msg,sizeof(Message),0, (struct sockaddr *)&client.socket_addr, sizeof(client.socket_addr));
}
void envoyerMessageSalon(struct Salon salon, struct Message msg){
    struct Client c;
    for (int i = 0; i < salon.nbClient; i++) {
        c=salon.clients[i];
        envoyerMessageClient(c,msg);
    }
}
/*
int startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
            lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}*/
