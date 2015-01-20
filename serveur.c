#include "protocole.h"
#include "serveur.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]){


Message msg;
msg.message="VIDE";
msg.salonCible="Accueil";

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
      printf("USER a demandé la liste des salons ouvert");
      listSalon();
      break;
    case Commande.HELP:
      printf("USER a demandé de l'aide");
      break;
    default:
      printf("Commande non reconnue");
      break;
    }



}

char* listSalon(){

}

char* listCommande(){
    char* ret="Commande : \n";
    char* serveur =
    char* nick =
    char* join =
    char* part =
    char* quit =
    char* list =
    char* help =
    strcat(ret,);
}
