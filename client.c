#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <regex.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "protocole.h"

void clean(const char *buffer, FILE *fp);

/**
* Socket client
*/
int sckt = -1;

/**
* Structure de gestion d'adresses cliente et serveur
*/
struct sockaddr_in client_addr, serv_addr;

/**
* Thread d'écoute serveur
*/
pthread_t ecoute;

/**
* Mutex d'accès aux ressources
*/
pthread_mutex_t communication_mutex;

/**
* Communications
*/
int shmid;
Communication * communications;

void _log(char * message) {
    puts(message);
}

/**
* Envoie une communication avec le serveur avec un thread non bloquant
*/
void * thread_communicate(void * var) {
    char * data = (char *) var;
    char buffer[255];
    char target[3];

    pthread_mutex_lock(&communication_mutex);

    if (send(sckt, data, strlen(data) + 1, 0)) {
        perror("sendto");
        return NULL;
    }

    if (read(sckt, buffer, MAX_MESSAGE) > 0) {
        strncpy(target, buffer, 3);

        if (strcmp("ACK", buffer) == 0) {
            pthread_exit(NULL);
        }
    }

    pthread_mutex_unlock(&communication_mutex);

    return NULL;
}

/**
* Gère la connexion à un serveur de communication
*
* @var address  Adresse IP du serveur
* @var port     Port du serveur
*/
int server(char * address, char * port) {
    char * message = "CONNECT";

    if ((sckt = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        return 1;
    }

    if (inet_aton(address, &(serv_addr.sin_addr)) == 0) {
        printf("Invalid IP address format <%s>\n", address);
        return 1;
    }

    serv_addr.sin_port = htons(atoi(port));

    if (connect(sckt, (struct sockaddr *) &serv_addr, sizeof serv_addr) == 1) {
        perror("connect");
        return 1;
    }

    pthread_mutex_init(&communication_mutex, NULL);
    pthread_create(&ecoute, NULL, thread_communicate, message);

    free(message);
}

/**
* Fonction pour quitter le serveur
*/
void quit(void) {
    if (sckt == -1) {
        pthread_join(ecoute, NULL);


        close(sckt);
        sckt = -1;
    }
}

int main(int argc, char *argv[]) {
    char * input;
    char * token = "NOQUIT";
    char * address;
    char * port;
    char input2[MAX_MESSAGE];

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port        = htons(0);
    serv_addr.sin_family = AF_INET;

    _log("## Bienvenue ##");

    shmid = shmget(IPC_PRIVATE, sizeof(Communication), IPC_CREAT | 0666);

    while (strcmp(token, "QUIT") != 0) {
        _log("## Entrez une commande (/HELP pour recevoir de l'aide)");

        fgets(input2, sizeof(input2), stdin);
        clean(input2, stdin);

        input = strdup(input2);

        if (input[0] == '/') {
            input++;
        }

        token = strtok(input, " ");
        if (strcmp(token, "SERVER") == 0) {
            address = strtok(NULL, " ");
            port = strtok(NULL, " ");

            if (server(address, port)) {

            }
        } else {
            if (sckt != -1 && communicate(input)) {

            }
        }

        free(input);
    }

    _log("Exiting the application...");
    pthread_join(ecoute, NULL);

    return 0;
}

/**
 * Supprime le \n du buffer et de stdin
 */
void clean(const char *buffer, FILE *fp) {
    char *p = strchr(buffer,'\n');
    if (p != NULL)
        *p = 0;
    else
    {
        int c;
        while ((c = fgetc(fp)) != '\n' && c != EOF);
    }
}