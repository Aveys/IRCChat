#include "protocole.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <regex.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

void clean(const char *buffer, FILE *fp);

int sckt = -1, i;
struct sockaddr_in client_addr, serv_addr;

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

    serv_addr.sin_port = htons(port);
}

/**
*
*/
int communicate(char * data) {
    if (sendto(sckt, data, strlen(data) + 1, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("sendto");
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]){
    char command[MAX_MESSAGE];
    char message[MAX_MESSAGE];
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
            if (sckt && communicate(input)) {

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