#include "protocole.h"
#include "serveur.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_PORT 1500

int main(int argc, char *argv[]){
    int sd, n;
    socklen_t addr_len;
    struct sockaddr_in client_addr, server_addr;
    struct Message msg;

    // Create socket
    if ((sd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket creation");
        return 1;
    }
    // Bind it
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);
    if (bind(sd, (struct sockaddr *)&server_addr, sizeof server_addr) == -1)
    {
        perror("bind");
        return 1;
    }

    for (;;)
    {

        addr_len = sizeof(client_addr);
        n = recvfrom(sd, msg, sizeof(msg), 0, (struct sockaddr *)&client_addr, &addr_len);
        if {
            printf("Connection d'un nouveau client");
        }
    }
    return 0;
}