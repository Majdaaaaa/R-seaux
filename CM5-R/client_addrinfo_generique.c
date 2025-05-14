#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>

union adresse{
	char adr4[INET_ADDRSTRLEN];
	char adr6[INET6_ADDRSTRLEN];
};

union sockadresse{
	struct sockaddr_in sadr4;
	struct sockaddr_in6 sadr6;
};

#define SIZE_MESS 100
#define MESS "Occupez-vous du sens et les mots s'occuperont d'eux-mêmes.\n"


/****************** Affichage des adresses ****************/

void affiche_adresse(union sockadresse addr, int adrlen){
	union adresse buf;
	
	if(adrlen == sizeof(struct sockaddr_in)){
		inet_ntop(AF_INET, &(addr.sadr4.sin_addr), buf.adr4, sizeof(buf.adr4));
		printf("adresse serveur : IP: %s port: %d\n", buf.adr4, ntohs(addr.sadr4.sin_port));
	}else{
		inet_ntop(AF_INET6, &(addr.sadr6.sin6_addr), buf.adr6, sizeof(buf.adr6));
		printf("adresse serveur : IP: %s port: %d\n", buf.adr6, ntohs(addr.sadr6.sin6_port));
	}
}

void affiche_adresse_sockaddr(struct sockaddr addr, int adrlen){
	union sockadresse adrsock;
	
	if(adrlen == sizeof(struct sockaddr_in))
		adrsock.sadr4 = *((struct sockaddr_in *) &addr);
	else
		adrsock.sadr6 = *((struct sockaddr_in6 *) &addr);

	affiche_adresse(adrsock, adrlen);
}

/****************** Connexion par adresse de noms ****************/

int get_server_addr(char* hostname, char* port, int * sock, union sockadresse* addr, int* addrlen) {
	struct addrinfo hints, *r, *p;
	int ret;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((ret = getaddrinfo(hostname, port, &hints, &r))){
		fprintf(stderr, "erreur getaddrinfo : %s\n", gai_strerror(ret));
		return -1;
	}
 
	p = r;
	while( p != NULL ){
		if((*sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) > 0){
			if(connect(*sock, p->ai_addr, p->ai_addrlen) == 0)
				break;
			printf("connexion impossible sur :\n");
			affiche_adresse_sockaddr(*((struct sockaddr *) p->ai_addr), p->ai_addrlen);
			printf("\n");
			close(*sock);
		}

		p = p->ai_next;
	}

	if (p == NULL) return -2;

	//on stocke l'adresse de connexion
	*addrlen = p->ai_addrlen;
	if(p->ai_family == AF_INET)
		memcpy(&(addr->sadr4), (struct sockaddr_in *) p->ai_addr, p->ai_addrlen);
	else
		memcpy(&(addr->sadr6), (struct sockaddr_in6 *) p->ai_addr, p->ai_addrlen);
  
	//on libère la mémoire allouée par getaddrinfo 
	freeaddrinfo(r);
  
	return 0;
}


int main(int argc, char** args) {
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <hostname> <port>\n", args[0]);
		exit(1);
	}
    
	union sockadresse server_addr;
	int fdsock, adrlen;
    
	switch (get_server_addr(args[1], args[2], &fdsock, &server_addr, &adrlen)) {
	case 0: printf("Connexion réussie !\n"); break;
	case -1:
		fprintf(stderr, "Erreur: hote non trouve.\n");
		exit(1);
	case -2:
		fprintf(stderr, "Erreur: echec de creation de la socket.\n");
		exit(1);
	}

	affiche_adresse(server_addr, adrlen);

	//*** envoie d'un message ***
	int ecrit = send(fdsock, MESS, strlen(MESS), 0);
	if(ecrit <= 0){
		perror("erreur ecriture");
		exit(3);
	}

	//*** reception d'un message ***
	char buf[SIZE_MESS+1];
	memset(buf, 0, SIZE_MESS+1);
	int recu = recv(fdsock, buf, SIZE_MESS, 0);
	if (recu <= 0){
		perror("erreur lecture");
		exit(4);
	}
	printf("%s\n", buf);
    
	close(fdsock);
    
	return 0;
}


