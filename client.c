#include "protocole.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

void clean(const char *buffer, FILE *fp);

int main(int argc, char *argv[]){
    char command[255];
    char message[255];
    char input[255];

    puts("## Bienvenue ##");
    puts("## Entrez une commande (/HELP pour recevoir de l'aide)");

    fgets(input, sizeof(input), stdin);
    clean(input, stdin);

    sscanf(input, "/%s %s", command, message);

    printf("%s %s", command, message);
}

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