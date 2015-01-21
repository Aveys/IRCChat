/**
 *
 * @author Vincent Valot, Mohammed Zizah
 * @brief Implémentation d'un client pour un chat entre plusieurs autres clients
 *
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <Foundation/Foundation.h>

#include "protocole.h"

void clean(const char *buffer, FILE *fp);
int call_function(const char *name, char * param);

/**
* Structure d'une réponse de thread
*/
typedef struct Response {
    int code;
    char value[MAX_MESSAGE];
} Response;

/**
* Socket client
*/
int sckt = -1;

/**
* Structures de gestion d'adresses cliente et serveur
*/
struct sockaddr_in client_addr, serv_addr;
socklen_t addr_len;

/**
* Thread d'écoute serveur
*/
pthread_t ecoute;

/**
* Mutex d'accès aux ressources
*/
pthread_mutex_t communication_mutex;

/**
 * Nom du client
 */
char * nickname;

void _log(char * message, ...) {
    va_list argptr;
    va_start(argptr, message);
    vfprintf(stderr, message, argptr);
    va_end(argptr);
}

/**
* Envoie une communication avec le serveur avec un thread non bloquant
*/
void * thread_communicate(void * var) {
    Response * response = &(Response) {
        .code = 0
    };

    struct Message message;
    fd_set rfds;
    struct timeval tv;
    int retval;

    strcpy(message.message, (char *) var);
    char buffer[255];
    char target[3];

    FD_ZERO(&rfds);
    FD_SET(sckt, &rfds);

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    if (sendto(sckt, &message, sizeof(message) + 1, 0, (struct sockaddr *)&serv_addr, addr_len)) {
        perror("sendto");
        return NULL;
    }

    // Timeout de 5 secondes pour la réception d'un ACK
    response->code = select(sckt + 1, &rfds, NULL, NULL, &tv);

    if (response->code == -1) {
        perror("select()");
    } else if (response->code && FD_ISSET(sckt + 1, &rfds)) {
        if (recvfrom(sckt, &message, sizeof(Message), 0, (struct sockaddr *) &serv_addr, &addr_len) > 0) {
            strncpy(target, message.message, 3);

            if (strcmp("ACK", buffer) == 0) {
                strcpy(response->value, message.message);
                return response;
            }
        }
    }

    return response;
}

/**
 * Engage une communication
 */
void communicate(char * command, char * message) {
    void * retval;
    Response * response;
    char input[3];

    pthread_mutex_lock(&communication_mutex);

    pthread_create(&ecoute, NULL, thread_communicate, message);
    pthread_join(ecoute, &retval);

    response = (Response *) &retval;

    pthread_mutex_unlock(&communication_mutex);

    // Test de la valeur de retour
    if (response->code == 1) {
        _log("# Le message n'a pas été reçu par le serveur, voulez-vous le renvoyer ? (Yes/no)");
        fgets(input, sizeof(input), stdin);
        clean(input, stdin);

        if (input[0] != 'n' && input[0] != 'N') {
            communicate(command, message);
        }
    } else {
        call_function(command, response->value);
    }

    free(response);
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

    if (bind(sckt,(struct sockaddr *) &client_addr, sizeof client_addr) == -1) {
        perror("bind");
        return 1;
    }

    if (inet_aton(address, &(serv_addr.sin_addr)) == 0) {
        _log("# Le format de l'adresse serveur est invalide <%s>\n", address);
        return 1;
    }

    serv_addr.sin_port = htons(atoi(port));

    addr_len = sizeof(serv_addr);

    communicate("CONNECT", message);

    free(message);

    return 0;
}

/**
 * Fonction pour quitter le serveur
 */
void quit(void) {
    if (sckt != -1) {
        pthread_join(ecoute, NULL);

        close(sckt);
        sckt = -1;
    }
}

/**
* Handler d'un envoi de message Join
*/
void _joinHandler(char * var) {

}

void _partHandler(char * var) {

}

void _quitHandler(char * var) {

}

void _listHandler(char * var) {

}

void _nickHandler(char * var) {

}

void _helpHandler(char * var) {

}

/**
* Structure pour gérer différents handlers de manière transparente
*/
const static struct {
    const char *name;
    void (*function)(char *);
} function_map [] = {
        {"join", _joinHandler },
        {"part", _partHandler },
        {"quit", _quitHandler },
        {"list", _listHandler },
        {"help", _helpHandler },
        {"nick", _nickHandler }
};

int main(int argc, char *argv[]) {
    char * input;
    char * command = "NOQUIT";
    char * address;
    char * port;
    char input2[MAX_MESSAGE];

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port        = htons(0);
    serv_addr.sin_family = AF_INET;

    _log("## Bienvenue ##\n");
    _log("## Entrez une commande (/HELP pour recevoir de l'aide)\n");

    pthread_mutex_init(&communication_mutex, NULL);

    while (strcmp(command, "QUIT") != 0) {

        fgets(input2, sizeof(input2), stdin);
        clean(input2, stdin);

        input = strdup(input2);

        if (input[0] == '/') {
            input++;
        }

        command = strtok(input, " ");
        if (strcmp(command, "SERVER") == 0) {
            address = strtok(NULL, " ");
            port = strtok(NULL, " ");

            if (server(address, port) == 0) {
                _log("# Connexion au serveur %s port %s établie\n", address, port);
            } else {
                _log("# Connexion au serveur %s port %s échouée\n", address, port);
            }
        } else {
            if (sckt != -1) {
                communicate(command, input);
            }
        }

        free(input);
    }

    _log("Exiting the application...");
    quit();

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

/**
* Appelle un handler
*/
int call_function(const char *name, char * param) {
    int i;

    for (i = 0; i < (sizeof(function_map) / sizeof(function_map[0])); i++) {
        if (!strcmp(function_map[i].name, name) && function_map[i].function) {
            function_map[i].function(param);
            return 0;
        }
    }

    return -1;
}