CFLAGS= -Wall -Wextra -O2 -g -lpthread -std=c99
all: client serveur
serveur: serveur.o
	gcc $^ -o $@ ${CFLAGS}
client: client.o
	gcc $^ -o $@ ${CFLAGS}
clean:
	@rm *.o client serveur
