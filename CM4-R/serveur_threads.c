#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define SIZE_BUF 256

void *serve(void *arg) {
	int sock = *((int *) arg);
	char buf[SIZE_BUF];
	memset(buf, 0, sizeof(buf));
  
	int recu = recv(sock, buf, SIZE_BUF, 0);
	if (recu < 0){
		perror("recv");
		close(sock);
		free(arg);
		int *ret = malloc(sizeof(int));
		*ret = 1;
		pthread_exit(ret);
	}
	if(recu == 0){
		fprintf(stderr, "send du client nul\n");
		close(sock);
		free(arg);
		return NULL;
	}
	printf("recu : %s\n", buf);
	char c = 'o';
	int ecrit = send(sock, &c, 1, 0);
	if(ecrit <= 0)
		perror("send");

	close(sock);
	free(arg);
	return NULL;
}

int main(int argc, char *argv[]){
	//*** creation de l'adresse du destinataire (serveur) ***
	struct sockaddr_in6 address_sock;
	memset(&address_sock, 0, sizeof(address_sock));
	address_sock.sin6_family = AF_INET6;
	address_sock.sin6_port = htons(atoi(argv[1]));
	address_sock.sin6_addr = in6addr_any;

	//*** creation de la socket ***
	int sock = socket(PF_INET6, SOCK_STREAM, 0);
	if(sock < 0){
		perror("creation socket");
		exit(1);
	}


	//*** on lie la socket au port ***
	int r = bind(sock, (struct sockaddr *) &address_sock, sizeof(address_sock));
	if (r < 0) {
		perror("erreur bind");
		exit(2);
	}

	//*** Le serveur est pret a ecouter les connexions sur le port ***
	r = listen(sock, 0);
	if (r < 0) {
		perror("erreur listen");
		exit(2);
	}
  
	while(1){
		struct sockaddr_in6 addrclient;
		socklen_t size=sizeof(addrclient);
    
		//*** on crée la varaiable sur le tas ***
		int *sock_client = malloc(sizeof(int));

		//*** le serveur accepte une connexion et initialise la socket de communication avec le client ***
		*sock_client = accept(sock, (struct sockaddr *) &addrclient, &size);

		if (sock_client >= 0) {
			pthread_t thread;
			//*** le serveur cree un thread et passe un pointeur sur socket client à la fonction serve ***
			if (pthread_create(&thread, NULL, serve, sock_client)) {
				perror("pthread_create");
				continue;
			}  
			//*** affichage de l'adresse du client ***
			char nom_dst[INET6_ADDRSTRLEN];
			printf("client connecte : %s %d\n", inet_ntop(AF_INET6,&addrclient.sin6_addr,nom_dst,sizeof(nom_dst)), htons(addrclient.sin6_port));
		}
	}

	//*** fermeture socket serveur ***
	close(sock);
  
	return 0;
}
