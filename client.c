#include "protocole.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <regex.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <QTKit/QTKit.h>

void clean(const char *buffer, FILE *fp);

int main(int argc, char *argv[]){
    char command[MAX_MESSAGE];
    char message[MAX_MESSAGE];
    char * input;
    char * token;
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
    }
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