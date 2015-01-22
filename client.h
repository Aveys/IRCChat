

#define TIMEOUT 10

/**
* Structure d'une réponse de thread
*/
typedef struct Communication {
    int code;
    struct Message response;
    struct Message request;
} Communication;

void clean(const char *, FILE *);

int call_function(const char *, Communication *);

int isCommand(const char *);

char * getPartOfCommand(const char *, int);

void prepend(char *, const char *);

int communicate(const char *, char *);

void * thread_communicate(void *);

void * thread_alive(void *);

void _log(const char *, ...);

void _debug(const char *, ...);

int server(const char *, const char *);

int isCommand(const char *);

void clean(const char *, FILE *);

void prepend(char *, const char *);

int call_function(const char *, Communication *);

void addSalon(char *);

void removeSalon(char *);