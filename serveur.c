#include "protocole.h"
#include "serveur.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_PORT 1500

int main(int argc, char *argv[]) {
    int sd, n;
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
    server_addr.sin_port = htons(SERVER_PORT);
    if (bind(sd, (struct sockaddr *) &server_addr, sizeof server_addr) == -1) {
        perror("bind");
        return 1;
    }

    for (; ;) {
        addr_len = sizeof(client_addr);
        n = recvfrom(sd, msg, sizeof(msg), 0, (struct sockaddr *) &client_addr, &addr_len);
        if (str.cmp(msg.message, "CONNECT")) {
            print("Connection d'un nouveau client : %s", inet_ntoa(client_addr.sin_addr));
        }
        Message msg;
        msg.message = "VIDE";
        msg.salonCible = "Accueil";


    switch (msg.message)
    {
        case Commande.SERVER:
          printf("USER demande la connexion au serveur");
          break;
        case Commande.NICK:
          printf("USER a changé de pseudonyme");
          break;
        case Commande.JOIN:
          printf("USER a demande à rejoindre SALON");
          break;
        case Commande.PART:
          printf("USER a quitté le SALON");
          break;
        case Commande.QUIT:
          printf("USER s'est déconnecté");
          break;
        case Commande.LIST:
          printf("USER a demandé la liste des salons ouverts");
          listSalon();
          break;
        case Commande.HELP:
          printf("USER a demandé de l'aide");
          listCommandes();
          break;
        default:
          printf("Commande non reconnue");
          break;

        }
    }
}

char* listSalon(){
}

char* listCommandes(){
    char* ret="Commande : \n - /SERVER <@IP>: Demander la connexion au serveur \n - /NICK <pseudonyme>: Changer de pseudonyme \n - /JOIN <Salon>: Rejoindre un serveur \n - /PART : Quitter le salon \n - /LIST : Lister les salons ouverts \n - /HELP : Afficher la liste des commandes possibles\n";
}
