#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <string.h>
#include <unistd.h>

#define PORT 13
#define ADDR "127.0.0.0"

int main(){
    // crée la socket
    // préparer l'adresse
    // faire une demande

    int fdsock = socket(PF_INET, SOCK_STREAM, 0);

    if(fdsock == -1){
		perror("creation socket");
		exit(1);
	}

    struct sockaddr_in adrso;
    // ! toujours remplir la structure de 0
    memset(&adrso,0,sizeof(adrso));

    adrso.sin_family = AF_INET;
    // ! toujours convertir en big-endian
    adrso.sin_port = htons(PORT);

    if(inet_pton(AF_INET,ADDR,&adrso.sin_addr)!=1){
        perror("adresse pas traduite");
        exit(1);
    }

    if((connect(fdsock,(struct sockaddr *)&adrso,sizeof(adrso)))<0){
        perror("connect");
        exit(1);
    }

    char buf[1000];
    int r = recv(fdsock,buf,99*sizeof(char),0);
    buf[r]='\0';

    printf("%s\n",buf);

    close(fdsock);
    exit(0);

}