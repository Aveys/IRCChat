/**
*
* @author Vincent Valot, Mohammed Zizah
* @brief Implémentation d'un client pour un chat entre plusieurs autres clients
*
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <regex.h>

#include "protocole.h"
#include "client.h"

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
pthread_attr_t communication_pthread_attr;

/**
* Thread d'écoute serveur
*/
pthread_t alive_pthread;
pthread_attr_t alive_pthread_attr;

/**
* Nom du client
*/
char *nickname = "Anonymous";

/**
* Salon courant du client
*/
char ** salon;

void sigint(int signal) {
    _log("\n# Fermeture de l'application... Veuillez patienter...\n");
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    char *input;
    char *command = "NOQUIT";
    char *address;
    char *port;
    char input2[MAX_MESSAGE];

    _log("# Bienvenue");
    if (argc > 1) {
        if (strcmp("--debug", argv[1]) == 0) {
            debug = 1;
            _log(" (debug is active)");
        }
    }

    signal(SIGINT, sigint);

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port = htons(0);
    serv_addr.sin_family = AF_INET;

    _log("\n# Entrez une commande (/HELP pour recevoir de l'aide)\n");

    do {
        fgets(input2, sizeof(input2), stdin);
        clean(input2, stdin);

        if (strlen(input2)) {
            input = strdup(input2);

            if (isCommand(input)) {
                if (input[0] == '/') {
                    input++;
                }

                command = getPartOfCommand(input, 1);

                if (command && strcmp(command, "SERVER") == 0) {
                    command = strtok(input, " ");
                    address = strtok(NULL, " ");
                    port = strtok(NULL, " ");

                    if (address && port) {
                        if (server(address, port) == 0) {
                            _log("# Connexion au serveur %s port %s établie\n", address, port);

                            pthread_attr_init(&alive_pthread_attr);
                            pthread_attr_setdetachstate(&alive_pthread_attr, PTHREAD_CREATE_JOINABLE);

                            pthread_create(&alive_pthread, &alive_pthread_attr, thread_alive, NULL);
                        } else {
                            _log("# Connexion au serveur %s port %s échouée\n", address, port);
                        }
                    } else {
                        _log("# Utilisation de SERVER: /SERVER ip port\n");
                    }
                } else if (command && sckt != -1) {
                    communicate(command, input);
                }
            } else if (sckt != -1) {
                prepend(input, "MESSAGE ");
                communicate("MESSAGE", input);
            }

            if (sckt == -1) {
                _log("# Vous n'êtes connecté à aucun serveur\n");
            }
        }
    } while (strcmp(command, "EXIT") != 0);

    _log("# Fermeture de l'application\n");
    pthread_exit(NULL);

    return 0;
}

/**
* Fonction d'affichage des messages au client
* @var message  Message a affficher
* @var ...      variables à utiliser
*/
void _log(const char *message, ...) {
    va_list argptr;
    va_start(argptr, message);
    vfprintf(stderr, message, argptr);
    va_end(argptr);
}

void _debug(const char *message, ...) {
    if (debug == 1) {
        printf("# DEBUG: %s\n", message);
    }
}

/**
* Envoie une communication avec le serveur avec un thread non bloquant
*
* @var var La chaine de caractère à envoyer
* @return Communication
*/
void *thread_communicate(void *var) {
    Communication *communication = malloc(sizeof(Communication));
    communication->code = -1;

    struct Message message;
    fd_set rfds;
    struct timeval tv;

    strcpy(communication->request.message, (char *) var);
    char buffer[255];
    char target[3];
    int retval;

    FD_ZERO(&rfds);
    FD_SET(sckt, &rfds);

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    if (sendto(sckt, &communication->request, sizeof(communication->request) + 1, 0, (struct sockaddr *) &serv_addr, addr_len) == -1) {
        perror("sendto()");
    }

    // Timeout de 5 secondes pour la réception d'un ACK
    retval = select(sckt + 1, &rfds, NULL, NULL, &tv);

    if (retval == -1) {
        perror("select()");
    } else if (retval && FD_ISSET(sckt, &rfds)) {
        if (recvfrom(sckt, &communication->response, sizeof(communication->response), 0, (struct sockaddr *) &serv_addr, &addr_len) > 0) {
            strncpy(target, communication->response.message, 3);

            if (strcmp("ACK", buffer) == 0) {
                communication->code = 1;
            } else if (strcmp("ERR", buffer) == 0) {
                communication->code = 2;
            }
        }
    }

    return communication;
}

/**
* Thread d'envoi de ping au serveur
*
* @var var
*/
void *thread_alive(void *var) {
    Communication *response;

    while (sckt != -1) {
        response = thread_communicate((void *) "ALIVE");

        if (!response->code) {
            _log("# Connexion au serveur perdue...");
        }

        sleep(TIMEOUT);
    }

    return NULL;
}

/**
* Engage une communication
*
* @var command commande à executer
* @var message message à envoyer au serveur
*
* @return 0 on success, 1 on error
*/
int communicate(const char * command, char * message) {
    void *retval;
    Communication *response;
    char input[3];

    pthread_attr_init(&communication_pthread_attr);
    pthread_attr_setdetachstate(&communication_pthread_attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&communicate_pthread, &communication_pthread_attr, thread_communicate, message);
    pthread_join(communicate_pthread, &retval);

    pthread_attr_destroy(&communication_pthread_attr);

    response = (Communication *) retval;

    // Test de la valeur de retour
    if (response->code) {
        call_function(command, response);
        free(response);

        return 0;
    } else {
        _log("# Le message n'a pas été reçu par le serveur, voulez-vous le renvoyer ? (Yes/no)");
        fgets(input, sizeof(input), stdin);
        clean(input, stdin);

        if (input[0] != 'n' && input[0] != 'N') {
            communicate(command, message);
        }
    }

    free(response);

    return 1;
}

/**
* Gère la connexion à un serveur de communication
*
* @var address  Adresse IP du serveur
* @var port     Port du serveur
*/
int server(const char *address, const char *port) {
    char *message = "CONNECT";

    if ((sckt = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        return 1;
    }

    if (bind(sckt, (struct sockaddr *) &client_addr, sizeof client_addr) == -1) {
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
void _joinHandler(const Communication *communication) {
    char * tmp = getPartOfCommand(&communication->request.message[0], 2);

    if (strlen(tmp) > 0) {
        _log("# Vous avez bien rejoint le salon <%s>\n", salon);
    } else {
        _log("# Vous avez bien rejoint le salon\n");
    }
}

void _partHandler(const Communication *communication) {
    if (strcmp("ACK_PART", communication->response.message)) {
        _log("# Vous avez bien quitté le salon\n");
    } else if (strcmp("ERR_NOCHANNELJOINED", communication->response.message)) {
        _log("# Vous avez demandé à quitter un salon que vous n'aviez pas rejoint\n");
    }
}

/**
* Handler pour quitter un serveur
*/
void _quitHandler(const Communication *communication) {
    if (sckt != -1) {
        _log("# Fermeture de la connexion serveur...\n");
        pthread_join(communicate_pthread, NULL);
        pthread_join(alive_pthread, NULL);

        pthread_attr_destroy(&alive_pthread_attr);

        close(sckt);
        sckt = -1;
    }
}

void _listHandler(const Communication *communication) {
}

void _nickHandler(const Communication *communication) {
    char *tmp = getPartOfCommand(communication->request.message, 2);

    if (strcmp("ACK_NICKMODIFIED", communication->response.message)) {
        nickname = strdup(tmp);
        _log("# Votre pseudonyme a bien été changé en <%s>\n", nickname);
    } else if (strcmp("ERR_NICKALREADYUSED", communication->response.message)) {
        _log("# Le pseudonyme <> demandé est déjà utilisé\n");
    }

    free(tmp);
}

void _messageHandler(const Communication *communication) {
    _debug("MESSAGE");

    _log("<%s> %s\n", nickname, communication->response.message);
}

void _helpHandler(const Communication *response) {
    _debug("HELP");
}

/**
* Structure pour gérer différents handlers de manière transparente
*/
const static struct {
    const char *name;

    void (*function)(const Communication *);
} function_map[] = {
        {"JOIN", _joinHandler},
        {"PART", _partHandler},
        {"QUIT", _quitHandler},
        {"LIST", _listHandler},
        {"HELP", _helpHandler},
        {"NICK", _nickHandler},
        {"MESSAGE", _messageHandler}
};

/**
* Vérifie si une chaine de caractères est une commande
*
* @var input
*/
int isCommand(const char *input) {
    int error;
    regex_t preg;
    const char *pattern = "^\\/[A-Z]+ ?.+";

    error = regcomp(&preg, pattern, REG_NOSUB | REG_EXTENDED);

    if (error == 0) {
        int match;

        match = regexec(&preg, input, (size_t) 0, NULL, 0);
        regfree(&preg);

        if (match == 0) {
            _debug("isCommand -> Match");
            return 1;
        } else {
            _debug("isCommand -> No match");
        }
    }

    return 0;
}

/**
* Retourne une partie d'une commande au format standard
*
* @var command La commande à découper
* @var part 1 pour la commande, 2 pour la valeur
*/
char *getPartOfCommand(const char *command, int part) {
    char *value = "";
    int error, start, end, isMatched, size;
    const char *pattern = "^([A-Z]+) ?(.*)";
    regex_t preg;
    regmatch_t * pmatch = NULL;

    error = regcomp(&preg, pattern, REG_EXTENDED);

    if (error == 0) {
        pmatch = malloc(sizeof(*pmatch) * (preg.re_nsub + 1));

        isMatched = regexec(&preg, command, preg.re_nsub, pmatch, 0);
        regfree(&preg);

        if (isMatched == 0) {
            _debug("getPartofCommand -> Match");

            start = (int) pmatch[1].rm_so;
            end = (int) pmatch[1].rm_eo;
            size = end - start;

            if (part == 2) {
                size = (int) strlen(command) - end - 1;
                start = end + 1;
            }

            value = malloc(sizeof(char) * (size + 1));

            if (value) {
                strncpy(value, &command[start], size);
                value[size] = '\0';
            }

            _log("# salon %d %d %d %s\n", start, end, size, value);
        } else {
            _debug("getPartofCommand -> No Match");
        }

        free(pmatch);
    }

    return value;
}

/**
* Supprime le \n du buffer et de stdin
*/
void clean(const char *buffer, FILE *fp) {
    char *p = strchr(buffer, '\n');
    if (p != NULL)
        *p = 0;
    else {
        int c;
        while ((c = fgetc(fp)) != '\n' && c != EOF);
    }
}

/**
* Prepends t into s. Assumes s has enough space allocated
* for the combined string.
*
* @var s La chaine devant être réécrite
* @var t La chaine devant être écrite devant s
*/
void prepend(char *s, const char *t) {
    size_t len = strlen(t);
    size_t i;

    memmove(s + len, s, strlen(s) + 1);

    for (i = 0; i < len; ++i) {
        s[i] = t[i];
    }
}

/**
* Appelle un handler
*/
int call_function(const char *name, Communication *param) {
    int i;

    for (i = 0; i < (sizeof(function_map) / sizeof(function_map[0])); i++) {
        if (!strcmp(function_map[i].name, name) && function_map[i].function) {
            function_map[i].function(param);
            return 0;
        }
    }

    return -1;
}