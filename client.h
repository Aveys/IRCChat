#define TIMEOUT 10

/**
* Structure d'une réponse de thread
*/
typedef struct Communication {
    int code;
    time_t requested;
    struct Message response;
    struct Message request;
} Communication;

void clean(const char *, FILE *);

int call_function(const char *, Communication *);

int isCommand(const char *);

char * getPartOfCommand(const char *, int);

void prepend(char *, const char *);

int communicate(const char *, char *);

void * thread_send(void *);

void * thread_alive(void *);

void *thread_listen(void *);

void _refreshScreen(void);

void _log(const char *, ...);

void _debug(const char *);

int server(const char *, const char *);

int isCommand(const char *);

void clean(const char *, FILE *);

void prepend(char *, const char *);

int call_function(const char *, Communication *);

void sigint(int signal);

void addSalon(char *);

void removeSalon(char *);

void addMessage(char *);

void _quitHandler(const Communication *);