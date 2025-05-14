#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE_MESS 100
#define NOM "Cerise"

void affiche_connexion(struct sockaddr_in adrclient){
	char adr_buf[INET_ADDRSTRLEN];
	memset(adr_buf, 0, sizeof(adr_buf));
  
	inet_ntop(AF_INET, &(adrclient.sin_addr), adr_buf, sizeof(adr_buf));
	printf("adresse client : IP: %s port: %d\n", adr_buf, ntohs(adrclient.sin_port));
}

int communication(int sockclient){
	//*** reception d'un message ***
	char buf[SIZE_MESS+1];
	memset(buf, 0, SIZE_MESS+1);
	int recu = recv(sockclient, buf, SIZE_MESS, 0);
	if (recu <= 0){
		perror("erreur lecture");
		return 1;
	}
	buf[recu] = '\0';
	printf("%s\n", buf);

	//*** envoie d'un message ***
	memset(buf, 0, SIZE_MESS+1);
	sprintf(buf, "Salut %s", NOM);
	int ecrit = send(sockclient, buf, strlen(buf), 0);
	if(ecrit <= 0){
		perror("erreur ecriture");
		return 2;
	}
	
	return 0;
}

int main(int argc, char** args) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <port>\n", args[0]);
		exit(1);
	}
    
	//*** creation de la socket serveur ***
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		perror("creation socket");
		exit(1);
	}

	//*** creation de l'adresse du destinataire (serveur) ***
	struct sockaddr_in address_sock; // = {.sin_family = AF_INET, .sin_port = htons(atoi(args[1])), .sin_addr.s_addr = htonl(INADDR_ANY)} ;
	memset(&address_sock, 0, sizeof(address_sock));
	address_sock.sin_family = AF_INET;
	address_sock.sin_port = htons(atoi(args[1]));
	address_sock.sin_addr.s_addr=htonl(INADDR_ANY);
  
	//*** on lie la socket au port PORT ***
	int r = bind(sock, (struct sockaddr *) &address_sock, sizeof(address_sock));
	if (r < 0) {
		perror("erreur bind");
		exit(2);
	}

	//*** Le serveur est pret a ecouter les connexions sur le port PORT ***
	r = listen(sock, 0);
	if (r < 0) {
		perror("erreur listen");
		exit(2);
	}

	//*** le serveur accepte une connexion et cree la socket de communication avec le client ***
	struct sockaddr_in adrclient;
	memset(&adrclient, 0, sizeof(adrclient));
	socklen_t size=sizeof(adrclient);
	int sockclient = accept(sock, (struct sockaddr *) &adrclient, &size);
	if(sockclient == -1){
		perror("probleme socket client");
		exit(1);
	}	   

	affiche_connexion(adrclient);
  
	communication(sockclient);

	//*** fermeture socket client ***
	close(sockclient);

	//*** fermeture socket serveur ***
	close(sock);
  
	return 0;
}
