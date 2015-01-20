typedef struct Args_Thread {
    struct sockaddr_in addr;
} Args_Thread;

char* listCommandes();
char* listSalon();
void *thread_client(void *arguments);
