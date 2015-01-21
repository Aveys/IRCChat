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
#include <regex.h>

#include "protocole.h"

void clean(const char *buffer, FILE *fp);
int call_function(const char *name, char * param);
int isCommand(const char * input);

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
* Variable de debug
*/
int debug = 0;

/**
* Structures de gestion d'adresses cliente et serveur
*/
struct sockaddr_in client_addr, serv_addr;
socklen_t addr_len;

/**
* Thread d'écoute serveur
*/
pthread_t communicate_pthread;

/**
* Thread d'écoute serveur
*/
pthread_t alive_pthread;

/**
 * Nom du client
 */
char * nickname;

void _log(const char * message, ...) {
    va_list argptr;
    va_start(argptr, message);
    vfprintf(stderr, message, argptr);
    va_end(argptr);
}

void _debug(const char * message, ...) {
    if (debug == 1) {
        printf("# DEBUG: %s\n", message);
    }
}

/**
* Envoie une communication avec le serveur avec un thread non bloquant
* @var var La chaine de caractère à envoyer
*/
void * thread_communicate(void * var) {
    Response * response = malloc(sizeof(Response));
    response->code = -1;

    struct Message message;
    fd_set rfds;
    struct timeval tv;

    strcpy(message.message, (char *) var);
    char buffer[255];
    char target[3];
    int retval;

    FD_ZERO(&rfds);
    FD_SET(sckt, &rfds);

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    if (sendto(sckt, &message, sizeof(message) + 1, 0, (struct sockaddr *)&serv_addr, addr_len) == -1) {
        perror("sendto()");
    }

    // Timeout de 5 secondes pour la réception d'un ACK
    retval = select(sckt + 1, &rfds, NULL, NULL, &tv);

    if (retval == -1) {
        perror("select()");
    } else if (retval && FD_ISSET(sckt, &rfds)) {
        if (recvfrom(sckt, &message, sizeof(Message), 0, (struct sockaddr *) &serv_addr, &addr_len) > 0) {
            strncpy(target, message.message, 3);
            strcpy(response->value, message.message);

            if (strcmp("ACK", buffer) == 0) {
                response->code = 1;
            } else if (strcmp("ERR", buffer) == 0) {
                response->code = 2;
            }
        }
    }

    return response;
}

/**
* Thread d'envoi de ping au serveur
*/
void * thread_alive(void * var) {
    Response * response;

    while (sckt != -1) {
        response = thread_communicate((void *) "ALIVE");

        if (!response->code) {
            _log("# Connexion au serveur perdue...");
        }

        sleep(10);
    }

    return NULL;
}

/**
 * Engage une communication
 */
int communicate(const char * command, char * message) {
    void * retval;
    Response * response;
    char input[3];

    pthread_create(&communicate_pthread, NULL, thread_communicate, message);
    pthread_join(communicate_pthread, &retval);

    response = (Response *) retval;

    // Test de la valeur de retour
    if (response->code) {
        call_function(command, response->value);
        return 0;
    } else {
        _log("# Le message n'a pas été reçu par le serveur, voulez-vous le renvoyer ? (Yes/no)");
        fgets(input, sizeof(input), stdin);
        clean(input, stdin);

        if (input[0] != 'n' && input[0] != 'N') {
            communicate(command, message);
        }
    }

    return 1;
}

/**
 * Gère la connexion à un serveur de communication
 *
 * @var address  Adresse IP du serveur
 * @var port     Port du serveur
 */
int server(const char * address, const char * port) {
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

    return communicate("CONNECT", message);
}

/**
* Handler d'un envoi de message Join
*/
void _joinHandler(const char * var) {
    _debug("JOIN");

    _log("# Vous avez bien rejoint le salon demandé\n");
}

void _partHandler(const char * var) {
    _debug("PART");

    if (strcmp("ACK_PART", var)) {
        _log("# Vous avez bien quitté le salon\n");
    } else if (strcmp("ERR_NOCHANNELJOINED", var)) {
        _log("# Vous avez demandé à quitter un salon que vous n'aviez pas rejoint\n");
    }
}

/**
* Handler pour quitter un serveur
*/
void _quitHandler(const char * var) {
    _debug("QUIT");

    if (sckt != -1) {
        _log("# Fermeture de la connexion serveur...\n");
        pthread_join(communicate_pthread, NULL);
        pthread_join(alive_pthread, NULL);

        close(sckt);
        sckt = -1;
    }
}

void _listHandler(const char * var) {
    _debug("LIST");
}

void _nickHandler(const char * var) {
    _debug("NICK");

    if (strcmp("ACK_NICKMODIFIED", var)) {
        _log("# Votre pseudonyme a bien été changé\n");
    } else if (strcmp("ERR_NICKALREADYUSED", var)) {
        _log("# Le pseudonyme demandé est déjà utilisé\n");
    }
}

void _messageHandler(const char * var) {
    _debug("MESSAGE");

    _log("<%s> %s\n", nickname, var);
}

void _helpHandler(const char * var) {
    _debug("HELP");
}

/**
* Structure pour gérer différents handlers de manière transparente
*/
const static struct {
    const char *name;
    void (*function)(const char *);
} function_map [] = {
        {"JOIN", _joinHandler },
        {"PART", _partHandler },
        {"QUIT", _quitHandler },
        {"LIST", _listHandler },
        {"HELP", _helpHandler },
        {"NICK", _nickHandler },
        {"MESSAGE", _messageHandler }
};

int main(int argc, char *argv[]) {
    char * input;
    char * command = "NOQUIT";
    char * address;
    char * port;
    char input2[MAX_MESSAGE];

    if (argc > 1) {
        if (strcmp("--debug", argv[1]) == 0) {
            debug = 1;
        }
    }

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port        = htons(0);
    serv_addr.sin_family = AF_INET;

    _log("# Bienvenue\n");
    _log("# Entrez une commande (/HELP pour recevoir de l'aide)\n");

    do {
        fgets(input2, sizeof(input2), stdin);
        clean(input2, stdin);

        if (strlen(input2)) {
            input = strdup(input2);

            if (isCommand(input)) {
                if (input[0] == '/') {
                    input++;
                }

                command = strtok(input, " ");
                if (strcmp(command, "SERVER") == 0) {
                    address = strtok(NULL, " ");
                    port = strtok(NULL, " ");

                    if (server(address, port) == 0) {
                        _log("# Connexion au serveur %s port %s établie\n", address, port);
                        pthread_create(&alive_pthread, NULL, thread_alive, NULL);
                    } else {
                        _log("# Connexion au serveur %s port %s échouée\n", address, port);
                    }
                } else if (sckt != -1) {
                    communicate(command, input);
                } else {
                    _log("# Vous n'êtes connecté à aucun serveur\n");
                }
            } else if (sckt != -1) {
                communicate("MESSAGE", input);
            } else {
                _log("# Vous n'êtes connecté à aucun serveur\n");
            }
        }
    } while (strcmp(command, "EXIT") != 0);

    _log("# Fermeture de l'application\n");

    return 0;
}

/**
* Vérifie si une chaine de caractères est une commande
*
* @var input
*/
int isCommand(const char * input) {
    int error;
    regex_t preg;
    const char * pattern = "^\\/[A-Z]+ ?.+";

    error = regcomp(&preg, pattern, REG_NOSUB | REG_EXTENDED);

    if (error == 0) {
        int match;

        match = regexec(&preg, input, (size_t) 0, NULL, 0);
        regfree(&preg);

        if (match == 0) {
            return 1;
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