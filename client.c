#include "protocole.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <regex.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

void clean(const char *buffer, FILE *fp);

/**
* @var socket client
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
* Communications
*/
Communication * communications;

void _log(char * message) {
    printf("%s\n", message);
}

/**
* Thread d'écoute serveur
*/
void * thread_process(void) {
    char * buffer;
    char target[3];
    int n;

    if ((n = read(sckt, buffer, MAX_MESSAGE)) > 0) {
        printf("Server: %s\n", buffer);

        strncpy(target, buffer, 3);

        // Si c'est un message d'acquittement
        if (strcmp(target, "ACK")) {

        } else {

        }
    }
}

/**
* Gère la connexion à un serveur de communication
*
* @var address  Adresse IP du serveur
* @var port     Port du serveur
*/
int server(char * address, char * port) {
    if ((sckt = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        return 1;
    }

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port        = htons(0);

    // Fill server address structure
    serv_addr.sin_family = AF_INET;

    if (inet_aton(address, &(serv_addr.sin_addr)) == 0) {
        printf("Invalid IP address format <%s>\n", address);
        return 1;
    }

    serv_addr.sin_port = htons(atoi(port));

    if (connect(sckt, (struct sockaddr *) &serv_addr, sizeof serv_addr) == 1) {
        perror("connect");
        return 1;
    }

    pthread_create(&ecoute, NULL, thread_process, NULL);
}

/**
* Envoie une communication avec le serveur
*/
int communicate(char * data) {
    if (sendto(sckt, data, strlen(data) + 1, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("sendto");
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    char * input;
    char * token;
    char * address;
    char * port;
    char input2[MAX_MESSAGE];

    puts("## Bienvenue ##");

    for (;;) {
        puts("## Entrez une commande (/HELP pour recevoir de l'aide)");

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
    }

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