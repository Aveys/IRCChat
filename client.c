/**
*
* @author Vincent Valot
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
int _socket = -1;

/**
* Variable de debug
*/
int _isDebug = 0;

/**
* Structures de gestion d'adresses cliente et serveur
*/
struct sockaddr_in _client_addr, _serv_addr;
socklen_t _addr_len;

/**
* Thread d'écoute serveur
*/
pthread_t _send_pthread;
pthread_attr_t _send_pthread_attr;

/**
* Thread d'envoie de ping
*/
pthread_t _alive_pthread;
pthread_attr_t _alive_pthread_attr;

/**
* Thread d'écoute serveur
*/
pthread_t _listen_pthread;
pthread_attr_t _listen_pthread_attr;

/**
* Thread de blocage communicate
*/
pthread_mutex_t _processing_mutex;

/**
* Nom du client
*/
char *_nickname = "Anonymous";

/**
* Salon courant du client
*/
char *_salon;

/**
* Salons courants du client
*/
char * _salons[10];
int _nbSalons = 0;

/**
* Communication en envoi
*/
Communication *_processing;

/**
* Messages sauvegardés
*/
char * _messages[20];
int _nbMessages = 0;

int main(int argc, char *argv[]) {
    char *input;
    char *command = "NOQUIT";
    char *address;
    char *port;
    char input2[MAX_MESSAGE];

    _log("# Bienvenue");
    if (argc > 1) {
        if (strcmp("--debug", argv[1]) == 0) {
            _isDebug = 1;
        }
    }

    pthread_mutex_init(&_processing_mutex, NULL);
    pthread_mutex_lock(&_processing_mutex);

    _client_addr.sin_family = AF_INET;
    _client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    _client_addr.sin_port = htons(0);
    _serv_addr.sin_family = AF_INET;

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
                        server(address, port);

                            pthread_attr_init(&_listen_pthread_attr);
                            pthread_attr_setdetachstate(&_listen_pthread_attr, PTHREAD_CREATE_JOINABLE);

                            _debug("main -> creating thread_listen");
                            pthread_create(&_listen_pthread, &_listen_pthread_attr, thread_listen, NULL);

                            if (communicate("CONNECT", "CONNECT") == 0) {
                                pthread_attr_init(&_alive_pthread_attr);
                                pthread_attr_setdetachstate(&_alive_pthread_attr, PTHREAD_CREATE_JOINABLE);

                                _debug("main -> creating thread_alive");
                                pthread_create(&_alive_pthread, &_alive_pthread_attr, thread_alive, NULL);

                                _log("# Connexion au serveur %s port %s établie\n", address, port);
                            } else {
                                _log("# Connexion au serveur %s port %s échouée\n", address, port);
                                pthread_mutex_unlock(&_processing_mutex);
                            }
                    } else {
                        _log("# Utilisation de SERVER: /SERVER ip port\n");
                    }
                } else if (command && _socket != -1) {
                    communicate(command, input);
                }
            } else if (_socket != -1) {
                prepend(input, "MESSAGE ");
                communicate("MESSAGE", input);
            }

            if (_socket == -1) {
                _log("# Vous n'êtes connecté à aucun serveur\n");
            }
        }
    } while (strcmp(command, "EXIT") != 0);

    _log("# Fermeture de l'application\n");
    _quitHandler(_processing);

    return 0;
}

/**
* Rafraichis l'écran
*/
void _refreshScreen(void) {
    printf("\033[1;1H\033[2J");

    if (_salon) {
        printf("# SALON ACTUEL \033[0;32m%s\033[0;37m\n", _salon);
    }

    if (_nbMessages > 0) {
        for (int i = _nbMessages - 1; i >= 0; i--) {
            printf("%s", _messages[i]);
        }
    }
}

/**
* Fonction d'affichage des messages au client
* @var message  Message a affficher
* @var ...      variables à utiliser
*/
void _log(const char *message, ...) {
    char str[512];

    va_list argptr;
    va_start(argptr, message);
    vsprintf(str, message, argptr);
    va_end(argptr);

    addMessage(str);
    _refreshScreen();
}

void _debug(const char *message) {
    time_t current;
    struct tm date;
    char str[512];
    char format[128];

    if (_isDebug == 1) {

        time(&current);
        date = *localtime(&current);

        strftime(format, 128, "%Y-%m-%d %H:%M:%S", &date);

        sprintf(str, "DEBUG\t%s\t%s\n", format, message);

        addMessage(str);

        _refreshScreen();
    }
}

/**
* Envoie une communication avec le serveur avec un thread non bloquant
*
* @var var La chaine de caractère à envoyer
* @return Communication
*/
void *thread_send(void *var) {
    _processing = malloc(sizeof(Communication));
    _processing->code = -1;

    strcpy(_processing->request.message, (char *) var);
    if (_salon) {
        strcpy(_processing->request.salonCible, _salon);
    }

    if (sendto(_socket, &_processing->request, sizeof(_processing->request) + 1, 0, (struct sockaddr *) &_serv_addr, _addr_len) == -1) {
        perror("sendto()");
    }

    _processing->requested = time(NULL);

    return NULL;
}

/**
* Teste le temps depuis l'émission de la requête procession, le cas échéant débloque le mutex de requete
*/
void testProcessingRequestTime() {
    if ((time(NULL) - _processing->requested) > 5) {
        // Si on a pas reçu d'ACK mais que la requête est toujours en attente, on débloque
        pthread_mutex_unlock(&_processing_mutex);
    }
}

/**
* Envoie une communication avec le serveur avec un thread non bloquant
*
* @var var La chaine de caractère à envoyer
* @return Communication
*/
void *thread_listen(void *var) {
    Communication * communication = malloc(sizeof(Communication));
    char target[4];
    char * command;

    while (_socket != -1) {
        _debug("thread_listen -> Waiting...");
        if (recvfrom(_socket, &communication->response, sizeof(communication->response), 0, (struct sockaddr *) &_serv_addr, &_addr_len) > 0) {
            _debug("thread_listen -> Received");
            _debug(communication->response.message);

            strncpy(target, communication->response.message, 3);
            target[3] = '\0';

            if (strcmp("ERR_CMDUNKNOWN", communication->response.message) == 0) {
                _log("# La commande n'est pas reconnue par le serveur\n");
            } else if (strcmp("ACK_ALIVE", communication->response.message) == 0) {
                testProcessingRequestTime();
            } else if (_processing) {
                _debug("thread_listen -> if _processing");
                _debug(target);
                if (strcmp("ACK", target) == 0) {
                    _debug("thread_listen -> ACK");
                    _processing->code = 1;
                    pthread_mutex_unlock(&_processing_mutex);
                } else if (strcmp("ERR", target) == 0) {
                    _debug("thread_listen -> ERR");
                    _processing->code = 2;
                    pthread_mutex_unlock(&_processing_mutex);
                } else {
                    testProcessingRequestTime();
                }
            } else {
                command = getPartOfCommand(communication->response.message, 1);
                call_function(command, communication);

                free(command);
            }
        }
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
    _debug("communicate -> beginning...");
    char input[3];

    pthread_attr_init(&_send_pthread_attr);
    pthread_attr_setdetachstate(&_send_pthread_attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&_send_pthread, &_send_pthread_attr, thread_send, (void *) message);
    pthread_join(_send_pthread, NULL);

    pthread_attr_destroy(&_send_pthread_attr);

    // On bloque ici en attente d'avoir l'ACK, si on ne reçois pas
    _debug("communicate -> Waiting for response");
    pthread_mutex_lock(&_processing_mutex);
    _debug("communicate -> Response acquired");

    // Test de la valeur de retour
    if (_processing->code) {
        call_function(command, _processing);

        free(_processing);
        return 0;
    } else {
        _log("# Le message n'a pas été reçu par le serveur, voulez-vous le renvoyer ? (Yes/no)");
        fgets(input, sizeof(input), stdin);
        clean(input, stdin);

        if (input[0] != 'n' && input[0] != 'N') {
            communicate(command, message);
        }
    }

    free(_processing);

    return 1;
}

/**
* Thread d'envoi de ping au serveur
*
* @var var
*/
void *thread_alive(void *var) {
    struct Message message;
    strcpy(message.message, "ALIVE");

    while (_socket != -1) {
        if (sendto(_socket, &message, sizeof(message) + 1, 0, (struct sockaddr *) &_serv_addr, _addr_len) == -1) {
            perror("sendto()");
        }

        sleep(10);
    }

    return NULL;
}

/**
* Gère la connexion à un serveur de communication
*
* @var address  Adresse IP du serveur
* @var port     Port du serveur
*/
int server(const char *address, const char *port) {
    if ((_socket = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        return 1;
    }

    if (bind(_socket, (struct sockaddr *) &_client_addr, sizeof _client_addr) == -1) {
        perror("bind");
        return 1;
    }

    if (inet_aton(address, &(_serv_addr.sin_addr)) == 0) {
        _log("# Le format de l'adresse serveur est invalide <%s>\n", address);
        return 1;
    }

    _serv_addr.sin_port = htons(atoi(port));

    _addr_len = sizeof(_serv_addr);

    return 0;
}

/**
* Handler d'un envoi de message
* @var Communication la communication liée
*/
void _joinHandler(const Communication *communication) {
    char * tmp = getPartOfCommand(communication->request.message, 2);

    _nbMessages = 0;

    if (strlen(tmp) > 0) {
        _salon = strdup(tmp);
        addSalon(_salon);
        free(tmp);
        _log("# Vous avez bien rejoint le salon <%s>\n", _salon);
    } else {
        _log("# Vous avez bien rejoint le salon\n");
    }
}


/**
* Handler d'un envoi de message
* @var Communication la communication liée
*/
void _partHandler(const Communication *communication) {
    if (strcmp("ACK_PART", communication->response.message)) {
        removeSalon(_salon);

        _log("# Vous avez bien quitté le salon <%s>\n", _salon);
    }
}

/**
* Handler d'un envoi de message
* @var Communication la communication liée
*/
void _quitHandler(const Communication *communication) {
    if (_socket != -1) {
        close(_socket);
        _socket = -1;

        _log("# Fermeture de la connexion serveur...\n");
        pthread_join(_send_pthread, NULL);
        pthread_join(_alive_pthread, NULL);
        pthread_join(_listen_pthread, NULL);

        pthread_attr_destroy(&_send_pthread_attr);
        pthread_attr_destroy(&_alive_pthread_attr);
        pthread_attr_destroy(&_listen_pthread_attr);
    }
}

/**
* Handler d'un envoi de message
* @var Communication la communication liée
*/
void _listHandler(const Communication *communication) {
    char * salons = getPartOfCommand(communication->request.message, 2);

    _log("Les salons disponibles sont: %s", salons);
}

/**
* Handler d'un envoi de message
* @var Communication la communication liée
*/
void _nickHandler(const Communication *communication) {
    char *tmp = getPartOfCommand(communication->request.message, 2);

    if (strcmp("ACK_NICKMODIFIED", communication->response.message)) {
        _nickname = strdup(tmp);
        _log("# Votre pseudonyme a bien été changé en <%s>\n", _nickname);
    } else if (strcmp("ERR_NICKALREADYUSED", communication->response.message)) {
        _log("# Le pseudonyme <> demandé est déjà utilisé\n");
    }

    free(tmp);
}

/**
* Handler d'un envoi de message
* @var Communication la communication liée
*/
void _messageHandler(const Communication *communication) {
    _debug("MESSAGE");
    time_t current;
    struct tm date;
    char format[128];

    time(&current);
    date = *localtime(&current);

    strftime(format, 128, "%H:%M", &date);

    if (strcmp("ERR_NOCHANNELJOINED", communication->response.message) == 0) {
        _log("# Vous n'avez pas rejoint de salon\n");
    } else {
        char * message = getPartOfCommand(communication->request.message, 2);

        _log("%s <\033[1m\033[41m%s\033[0;37m> %s\n", format, _nickname, message);

        free(message);
    }
}


/**
* Handler de réception de communication
* @var Communication la communication liée
*/
void _messagedHandler(const Communication *communication) {
    _debug("MESSAGED");

    time_t current;
    struct tm date;
    char format[128];

    if (strcmp(communication->response.salonCible, _salon) == 0) {
        time(&current);
        date = *localtime(&current);

        strftime(format, 128, "%H:%M", &date);

        char * message = getPartOfCommand(communication->request.message, 2);
        char * nickname;
        char * content;

        nickname = strtok(message, " ");
        content = strtok(NULL, " ");

        _log("%s <\033[1m\033[41m%s\033[0;37m> %s\n", format, nickname, content);
    }
}

/**
* Handler de réception de communication
* @var Communication la communication liée
*/
void _helpHandler(const Communication *communication) {
    _debug("HELP");

    char * message = getPartOfCommand(communication->request.message, 2);

    _log("# Commandes disponible: %s", message);

    free(message);
}

/**
* Handler de réception de communication
* @var Communication la communication liée
*/
void _chanJoinedHandler(const Communication *communication) {
    _debug("CHANJOINED");
    char * user;

    if (strcmp(communication->response.salonCible, _salon) == 0) {
        user = getPartOfCommand(communication->request.message, 2);

        _log("# Le salon a été rejoint par %s \n", user);

        free(user);
    }
}


/**
* Handler de réception de communication
* @var Communication la communication liée
*/
void _chanLeavedHandler(const Communication *communication) {
    _debug("CHANLEAVED");
    char * user;

    if (strcmp(communication->response.salonCible, _salon) == 0) {
        user = getPartOfCommand(communication->request.message, 2);

        _log("# Le salon a été quitté par %s \n", user);

        free(user);
    }

    _log("# Le salon a été quitté par %s\n", user, communication->response.message);
}


/**
* Handler de réception de communication
* @var Communication la communication liée
*/
void _nickModifiedHandler(const Communication *communication) {
    _debug("NICKMODIFIED");

    char * message;
    char * old;
    char * new;

    if (strcmp(communication->response.salonCible, _salon) == 0) {
        message = getPartOfCommand(communication->request.message, 2);

        old = strtok(message, " ");
        new = strtok(NULL, " ");

        _log("# L'utilisateur %s est maintenant connu sous le nom de %s\n", old, new);

        free(message);
    }
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
        {"MESSAGE", _messageHandler},
        {"CHANJOINED", _chanJoinedHandler},
        {"CHANLEAVED	", _chanLeavedHandler},
        {"MESSAGED", _messagedHandler},
        {"NICKMODIFIED", _nickModifiedHandler}
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

            if (value && size > 0) {
                strncpy(value, &command[start], size);
                value[size] = '\0';
            }
        } else {
            _debug("getPartofCommand -> No Match");
        }

        free(pmatch);
    }

    return value;
}

void addMessage(char * message) {
    if (_nbMessages > 0) {
        for (int i = _nbMessages - 1; i > 0; i--) {
            free(_messages[i]);
            _messages[i] = strdup(_messages[i - 1]);
        }
    } else {
        free(_messages[0]);
    }

    _messages[0] = strdup(message);

    if (_nbMessages < 20) {
        _nbMessages++;
    }
}

/**
* Recherche un salon dans la liste de salon
* @var salon Le salon a rechercher
*/
int searchSalon(char * salon) {
    int i = -1;

    for (i = 0; i < _nbSalons; i++) {
        if (strcmp(_salons[i], salon) == 0) {
            return i;
        }
    }

    return i;
}

/**
* Supprime un salon de la liste de salons
* @var salon Le salon à supprimer
*/
void removeSalon(char * salon) {
    int i = searchSalon(salon);

    if (i >= 0) {
        for(i = i; i < _nbSalons; i++) {
            _salons[i] = _salons[i+1];
        }

        _nbSalons--;
    }
}

/**
* Ajoute un salon à la lise de salon
* @var salon Le salon à ajouter
*/
void addSalon(char * salon) {
    if (_nbSalons < 10) {
        _salons[_nbSalons] = malloc(sizeof(char) * (strlen(salon) + 1));
        strcpy(_salons[_nbSalons], salon);
        _nbSalons++;
    }
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