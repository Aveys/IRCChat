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

struct Client clients[MAX_CLIENTS];//tableau des clients connecté
int nbClients=0;
struct Salon salons[MAX_SALONS];//tableau des salons crées
int nbSalons=0;
int sd;

int main(int argc, char *argv[]) {
    //INITIALISATION DES VARIABLES GLOBALES
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

        if (startsWith("CONNECT",msg.message) == 1){ // Vérification que le message commence par CONNECT
            printf("----- Demande de connection d'un nouveau client : %s -------\n", inet_ntoa(client_addr.sin_addr)); //DEBUG
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
                envoyerMessageClient(tmp.socket_addr, msg);
            }
            else{
                strcpy(tmp.pseudo,"ANONYME");
                tmp.socket_addr=client_addr;
                strcpy(msg.message,"ERR_IPALREADYUSED");
                strcpy(msg.salonCible,"");
                envoyerMessageClient(client_addr, msg);
            }
        }
        else if (startsWith("NICK",msg.message) == 1) {
            printf("----- Demande de Changement de Pseudonyme ------\n");
            int indice = findClient(client_addr);
            printf("INDICE CLIENT : %i\n",indice);

            int indiceSalon=-1;
            char *nickname;
            char *messageRetour;
            char *ancienPseudo;

            printClients(); //DEBUG : affiche liste des clients
            printf("Pseudo actuel : %s\n",clients[indice].pseudo);
            strcpy(ancienPseudo,clients[indice].pseudo);
            printf("Ancien pseudo : %s\n",ancienPseudo);//SEGMENTFAULT
            strcpy(nickname,msg.message);
            nickname+=5;
            printf("Changement de pseudonyme de %s en %s\n",clients[indice].pseudo,nickname);
            strcpy(clients[indice].pseudo,nickname);// on modifie le client dans la structure client
            for (int i = 0; i < nbSalons; ++i) {
                if( (indiceSalon = trouverClientDansSalon(salons[i], client_addr))!= -1){
                    printf("Utilisateur trouvé dans le salon %s\n",salons[indiceSalon].name);
                    strcpy(salons[i].clients[indiceSalon].pseudo,nickname);// on modifie le nickname dans la structure du salon
                    sprintf(messageRetour,"NICKMODIFIED %s %s",ancienPseudo,nickname);
                    //printf("Envoi de la trame : %s\n",messageRetour);
                    strcpy(msg.message,messageRetour);
                    strcpy(msg.salonCible,salons[i].name);
                    envoyerMessageSalon(salons[i],msg);
                }

            }
            printf("Envoi de la trame d'ACK_NICKMODIFIED\n");
            strcpy(msg.message,"ACK_NICKMODIFIED");//envoi de la notification de succes au client
            strcpy(msg.salonCible,"");
            envoyerMessageClient(client_addr, msg);
        }
        else if (startsWith("JOIN",msg.message) == 1) {
            printf("------- Demande de d'acces à un salon -------\n");
            int indiceClient = findClient(client_addr);
            char* nomSalon;
            strcpy(nomSalon,msg.message);
            nomSalon+=5;
            printf("L'utilisateur %s veut acceder au salon %s \n",clients[indiceClient].pseudo,nomSalon);
            joindreSalon(nomSalon,clients[indiceClient]);
            strcpy(msg.message,"ACK_JOIN");
            strcpy(msg.salonCible,nomSalon);
            envoyerMessageClient(client_addr, msg);
        }
        else if (startsWith( "MESSAGE",msg.message) == 1) {
            int indiceSalon = findSalon(msg.salonCible);
            int indiceClient = findClient(client_addr);
            char *message,*messageRetour;
            strcpy(message,msg.message);
            message+=8;
            sprintf(messageRetour,"MESSAGED %s %s",clients[indiceClient].pseudo,message);
            strcpy(msg.message,messageRetour);
            envoyerMessageSalon(salons[indiceSalon], msg);
            strcpy(msg.message,"ACK_MESSAGE");
            envoyerMessageClient(client_addr, msg);

        }
        else if (startsWith( "PART",msg.message) == 1) {

            fputs("USER a quitté le SALON", stdout);
        }
        else if (startsWith("QUIT",msg.message) == 1) {
            //VERIFIER PRESENCE DANS SALON -> SUPPRIMER DES SALONS

            //supprimerClient(); SUPPRIMER DU SERVEUR
        }
        else if (startsWith("LIST",msg.message) == 1) {
            fputs("USER a demandé la liste des salons ouvert", stdout);
            msg = listeSalon();
            envoyerMessageClient(client_addr, msg);
        }
        else if (startsWith("HELP",msg.message) == 1) {
            fputs("USER a demandé de l'aide", stdout);
            msg = listeSalon();
            envoyerMessageClient(client_addr, msg);
        }
        else {
            fputs("Commande non reconnue", stdout);
            msg = erreurCommande();
            envoyerMessageClient(client_addr, msg);
        }
    }
}
//renvoi l'indice du client dans le tableau
int findClient(struct sockaddr_in adresse) {
    struct sockaddr_in c;
    for (int i = 0; i < nbClients; i++) {
        c = clients[i].socket_addr;
        if (strcmp(inet_ntoa(adresse.sin_addr), inet_ntoa(c.sin_addr)) == 0) {
            printf("Client trouvé\n");
            return i;
        }
    }
    printf("Utilisateur %s introuvable\n", inet_ntoa(adresse.sin_addr));
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
struct Client getCurrentClient(struct sockaddr_in adresse){
    int indice = findClient(adresse);
    return clients[indice];
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



int findSalon(char *nomSalon){
    int indice = -1;
    for(int i=0;i<nbSalons;i++){
        if(strcmp(salons[i].name,nomSalon)==0)
           return i;
    }
    return indice;
}
struct Message joindreSalon(char* nomSalon,struct Client c){
    //Vérifier si le salon existe
    int existe=0;
    Salon s;
    existe = findSalon(nomSalon);
    //SI n'existe pas ALORS création du salon
    if(existe == -1){
        printf("Salon inexistant : creation \n");
        strcpy(s.name,nomSalon);
        salons[nbSalons]=s;
        existe = nbSalons;
        nbSalons++;
    }
    //Ajout du client au salon
    s.clients[0]=c;
    s.nbClient++;

    //Informer autres clients
    struct Message ret;
    strcpy(ret.salonCible,s.name);
    strcpy(ret.message,"CHANJOINED ");
    strcat(ret.message,c.pseudo);
    envoyerMessageSalon(s, ret); // Envoi du message à tous les clients du salon

    strcpy(ret.message,"ACK_JOIN"); // Message de retour client
    strcpy(ret.salonCible,"NULL");
    return ret;
}

void envoyerMessageClient(struct sockaddr_in adresse,struct Message msg){
    time_t rawtime;
    printf("# Envoi de la trame : %s avec salon %s à l'utilisateur en %s #\n",msg.message,msg.salonCible,inet_ntoa(adresse.sin_addr));
    time( &rawtime );
    msg.t=rawtime;
    sendto(sd,&msg,sizeof(Message),0, (struct sockaddr *)&adresse, sizeof(adresse));
}
// Envoi un message à tous les utilisateurs d'un salon
void envoyerMessageSalon(struct Salon salon, struct Message msg){
    printf("# Envoi de la trame : %s avec salon %s aux utilisateur(%i) du salon %s #\n",msg.message,msg.salonCible,salon.nbClient,salon.name);
    struct Client c;
    for (int i = 0; i < salon.nbClient; i++) {
        c=salon.clients[i];
        envoyerMessageClient(c.socket_addr,msg);
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

int startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
            lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}
void printClients(){
    for (int i = 0; i < nbClients; ++i) {
        Client c = clients[i];
        printf("CLIENT{%s %s %i}\t",c.pseudo,inet_ntoa(c.socket_addr.sin_addr),c.estVivant);

    }
    printf("\n");
}void printSalon(){
    for (int i = 0; i < nbSalons; ++i) {
        Salon s = salons[i];
        printf("Salon{%s %i}\t",s.name,s.nbClient);

    }
    printf("\n");
}
void decalageClient(int pos){
    for (int i = pos+1; i < nbClients; i++) {
        clients[i]=clients[i+1];
    }
}
void decalageSalon(int pos){
    for (int i = pos+1; i < nbSalons; i++) {
        salons[i]=salons[i+1];
    }
}
void decalageClientDansSalon(Salon s, int pos){

    for (int i = pos+1; i < s.nbClient; i++) {
        s.clients[i]=s.clients[i+1];
    }
}