
#include "protocole.h"
#include "serveur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT_SERVEUR 1500

//INITIALISATION DES VARIABLES GLOBALES
struct Client clients[MAX_CLIENTS];//tableau des clients connecté
int nbClients=0;
struct Salon salons[MAX_SALONS];//tableau des salons crées
int nbSalons=0;
int sd;
char time_serveur[80];

int main(int argc, char *argv[]) {


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

    struct Client tmp;//déclaration des variables utiles au fonctionnement A SUPPRIMER UNE FOIS LA FACTORISATION FAITE
    char *nickname;
    int indiceSalon=-1;
    char *messageRetour;
    char *ancienPseudo;
    int indice ;
    int indiceClient;
    char *nomSalon="";
    char *message="";
    printf("------ Lancement du serveur ------\n");
    for (; ;) {//TODO : factoriser tout ça
        // bloc de réception des clients
        getDateTime(time_serveur);
        printf("%sEn attente d'une trame\n",time_serveur);
        addr_len = sizeof(client_addr);
        n = recvfrom(sd,&msg, sizeof(Message), 0, (struct sockaddr *) &client_addr, &addr_len);
        if (n == -1)
            perror("recvfrom");
        getDateTime(time_serveur);
        printf("%sStructure reçu : %s, %s\n",time_serveur,msg.message,msg.salonCible); //DEBUG

        if (startsWith("CONNECT",msg.message) == 1){ // Vérification que le message commence par CONNECT
            getDateTime(time_serveur);
            printf("%s----- Demande de connection d'un nouveau client : %s -------\n",time_serveur, inet_ntoa(client_addr.sin_addr)); //DEBUG
            int indice = findClient(client_addr);
            if(indice==-1){
                getDateTime(time_serveur);
                printf("%sAjout du client\n",time_serveur);
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
            getDateTime(time_serveur);
            printf("%s----- Demande de Changement de Pseudonyme ------\n",time_serveur);
            indice = findClient(client_addr);
            printf("%sINDICE CLIENT : %i\n",time_serveur,indice);



            printClients(); //DEBUG : affiche liste des clients
            printf("%sPseudo actuel : %s\n",time_serveur,clients[indice].pseudo);
            strcpy(ancienPseudo,clients[indice].pseudo);
            //printf("%sAncien pseudo : %s\n",time_serveur,ancienPseudo);//SEGMENTFAULT
            strcpy(nickname,msg.message);
            nickname+=5;
            printf("%sChangement de pseudonyme de %s en %s\n",time_serveur,clients[indice].pseudo,nickname);
            strcpy(clients[indice].pseudo,nickname);// on modifie le client dans la structure client
            for (int i = 0; i < nbSalons; ++i) {
                if( (indiceSalon = trouverClientDansSalon(salons[i], client_addr))!= -1){
                    printf("%sUtilisateur trouvé dans le salon %s\n",time_serveur,salons[indiceSalon].name);
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
            printf("%s------- Demande de d'acces à un salon -------\n",time_serveur);
            indiceClient = findClient(client_addr);
            strcpy(nomSalon,msg.message);
            nomSalon+=5;
            struct Client c = clients[indiceClient];
            printf("%sL'utilisateur %s veut acceder au salon %s \n",time_serveur,clients[indiceClient].pseudo,nomSalon);
            joindreSalon(nomSalon,&c);
            strcpy(msg.message,"ACK_JOIN");
            strcpy(msg.salonCible,nomSalon);
            envoyerMessageClient(client_addr, msg);
        }
        else if (startsWith( "MESSAGE",msg.message) == 1) {
            indiceSalon = findSalon(msg.salonCible);
            indiceClient = findClient(client_addr);
            strcpy(message,msg.message);
            message+=8;
            sprintf(messageRetour,"MESSAGED %s %s",clients[indiceClient].pseudo,message);
            strcpy(msg.message,messageRetour);
            envoyerMessageSalon(salons[indiceSalon], msg);
            strcpy(msg.message,"ACK_MESSAGE");
            envoyerMessageClient(client_addr, msg);

        }
        else if (startsWith( "PART",msg.message) == 1) {
            quitterSalon(client_addr,msg.salonCible);
        }
        else if (startsWith("QUIT",msg.message) == 1) {
            quitterServeur(client_addr);
        }
        else if (startsWith("LIST",msg.message) == 1) {//TODO : debug
            getDateTime(time_serveur);
            printf("%s --------- L'utilisateur %s a demandé la liste des salons ouvert ---------", time_serveur,inet_ntoa(client_addr.sin_addr));
            listeSalon(&msg);
            envoyerMessageClient(client_addr, msg);
        }
        else if (startsWith("HELP",msg.message) == 1) {
            getDateTime(time_serveur);
            printf("%s-------- L'utilisateur %s à demandé la chaine de caractére d'aide -----------",time_serveur, inet_ntoa(client_addr.sin_addr));
            listeCommandes(&msg);
            envoyerMessageClient(client_addr, msg);
        }
        else if (startsWith("ALIVE",msg.message) == 1) {
            printf("%s------- L'utilisateur %s:%s a signalé qu'il était vivant mais je sais pas encore le gerer :( ---------",time_serveur,clients[indiceClient].pseudo,inet_ntoa(client_addr.sin_addr));
        }
        else {
            getDateTime(time_serveur);
            printf("%s!!!!!!!! Reception d'une commande inconnue : %s !!!!!!!!!!!",time_serveur,msg.message);
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

//Liste de tous les salons ouverts //TODO : tester
void listeSalon(struct Message *message){

    strcpy(message->salonCible,"");
    strcpy(message->message,"ACK_LIST");
    for(int i=0;i<nbSalons;i++){
        if(i!=0)
            strcat(message->message,", ");
        strcat(message->message,salons[nbSalons].name);
    }
}
//formate la liste des commandes
void listeCommandes(struct Message *msg){
    strcpy(msg->message,"ACK_HELP /SERVER <@IP>: Demander la connexion au serveur \n - /NICK <pseudonyme>: Changer de pseudonyme \n - /JOIN <Salon>: Rejoindre un serveur \n - /PART : Quitter le salon \n - /LIST : Lister les salons ouverts \n - /HELP : Afficher la liste des commandes possibles\n");
    strcpy(msg->salonCible,"");
}

struct Message erreurCommande(){
    struct Message ret;
    strcpy(ret.salonCible,"");
    strcpy(ret.message,"ERR_CMDUNKNOWN");
    return ret;
}
//cherche l'indice d'un salon par son nom, dans le tableau de salon. -1 si introuvable.
int findSalon(char *nomSalon){
    int indice = -1;
    for(int i=0;i<nbSalons;i++){
        if(strcmp(salons[i].name,nomSalon)==0)
           return i;
    }
    return indice;
}
//fonction pour joindre un salon et le créer si il n'existe pas
struct Message joindreSalon(char* nomSalon, struct Client *c){
    //Vérifier si le salon existe
    int existe=0;
    Salon s;
    existe = findSalon(nomSalon);
    //SI n'existe pas ALORS création du salon
    if(existe == -1){
        strcpy(s.name,nomSalon);
        s.nbClient=0;
        salons[nbSalons]=s;
        existe = nbSalons;
        nbSalons++;

    }
    s=salons[existe];
    //Informer autres clients
    struct Message ret;
    strcpy(ret.salonCible,s.name);
    strcpy(ret.message,"CHANJOINED ");
    strcat(ret.message,c->pseudo);
    envoyerMessageSalon(s, ret); // Envoi du message à tous les clients du salon
    //Ajout du client au salon
    salons[existe].clients[salons[existe].nbClient]=*c;
    salons[existe].nbClient++;
    strcpy(ret.message,"ACK_JOIN"); // Message de retour client
    strcpy(ret.salonCible,"NULL");
    return ret;
}

//envoi d'un message à un utilisateur précis
void envoyerMessageClient(struct sockaddr_in adresse,struct Message msg){
    time_t rawtime;
    printf("### %sEnvoi de la trame : %s avec salon %s à l'utilisateur en %s ###\n",time_serveur,msg.message,msg.salonCible,inet_ntoa(adresse.sin_addr));
    time( &rawtime );
    msg.t=rawtime;
    sendto(sd,&msg,sizeof(Message),0, (struct sockaddr *)&adresse, sizeof(adresse));
}
// Envoi un message à tous les utilisateurs d'un salon
void envoyerMessageSalon(struct Salon salon, struct Message msg){
    printf("### %sEnvoi de la trame : %s avec salon %s aux utilisateur(%i) du salon %s ###\n",time_serveur,msg.message,msg.salonCible,salon.nbClient,salon.name);
    struct Client c;
    for (int i = 0; i < salon.nbClient; i++) {
        c=salon.clients[i];
        envoyerMessageClient(c.socket_addr,msg);
    }
}

//Vérifie si un pseudonyme est libre d'etre utilisé
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

//Vérifie si une string commence par une sous-string
int startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
            lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}
// affiche la liste des clients connecter au serveur dans la console serveur
void printClients(){
    for (int i = 0; i < nbClients; ++i) {
        Client c = clients[i];
        printf("CLIENT{%s %s %i}\t",c.pseudo,inet_ntoa(c.socket_addr.sin_addr),c.estVivant);

    }
    printf("\n");
}
//Affiche dans la console serveur les salons et leurs nombres de clients
void printSalon(){
    for (int i = 0; i < nbSalons; ++i) {
        Salon s = salons[i];
        printf("Salon{%s %i}\t",s.name,s.nbClient);

    }
    printf("\n");
}
//reorganise par décalage à gauche du tableau de clients
void decalageClient(int pos){
    for (int i = pos+1; i < nbClients; i++) {
        clients[i]=clients[i+1];
    }
}
//reorganise par décalage à gauche du tableau de salon
void decalageSalon(int pos){
    for (int i = pos+1; i < nbSalons; i++) {
        salons[i]=salons[i+1];
    }
}
//reorganise par décalage à gauche du tableau de client d'un salon
void decalageClientDansSalon(Salon s, int pos){

    for (int i = pos+1; i < s.nbClient; i++) {
        s.clients[i]=s.clients[i+1];
    }
}
//Actualise la variable globale du temps
void getDateTime(char *t){
    time_t rawtime;
    struct tm *info;
    time( &rawtime );
    info = localtime( &rawtime );
    strftime(t,80,"%d/%m/%Y - %H:%M:%S : ", info);
}
//effectue les opérations pour deconnecter un utilisateur d'un salon et le supprime si il est vide
void quitterSalon(struct sockaddr_in adresse,char *salonCible) {
    int indice = findSalon(salonCible); //recupération de l'indice salon
    int indiceClient= findClient(adresse); // récupération de l'indice client


    if (indice != -1){
        char *reponse="";
        sprintf(reponse,"CHANLEAVED %s",clients[indiceClient].pseudo);
        struct Message message;
        strcpy(message.salonCible,salonCible);
        strcpy(message.message,reponse);
        envoyerMessageSalon(salons[indice], message);
        getDateTime(time_serveur);

        printf("%s-------- suppression de l'utilisateur %s du salon %s ----------",time_serveur,clients[indice].pseudo,salonCible);
        int indiceSalon=trouverClientDansSalon(salons[indice],adresse);
        decalageClientDansSalon(salons[indice], indiceSalon);
        salons[indice].nbClient--;

        if(salons[indice].nbClient == 0){
            getDateTime(time_serveur);
            printf("%s-------- Salon vide: supression du salon %s ----------",time_serveur,salonCible);
            decalageSalon(indice);
            nbSalons--;
        }
    }
    else{
        getDateTime(time_serveur);
        printf("%s-------- l'utilisateur %s est introuvable sur le salon %s ----------",time_serveur,clients[indice].pseudo,salonCible);
    }
}
//effectue les opération pour déconnecter un utilisateur (1: quitter les salons 2:quitter le serveur)
void quitterServeur(struct sockaddr_in adresse) {

    for (int i = 0; i < nbSalons; ++i) {
        quitterSalon(adresse, salons[i].name);//on quitte tous les salons
    }
    int indiceClient = findClient(adresse);

    if (indiceClient!=-1){
        getDateTime(time_serveur);
        printf("%s-------- Suppression de l'utilisateur %s du serveur ----------",time_serveur,clients[indiceClient].pseudo);
        decalageClient(indiceClient);
        nbClients--;
    }
    else{
        getDateTime(time_serveur);
        printf("%s-------- l'utilisateur %s est introuvable sur le serveur ----------",time_serveur, inet_ntoa(adresse.sin_addr));
    }

}
