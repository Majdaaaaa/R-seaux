#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <string.h>
#include <unistd.h>

#define PORT 37
#define ADDR "127.0.0.1"
int main()
{
    // crée la socket
    // préparer l'adresse
    // faire une demande

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        perror("creation coskcet");
        exit(1);
    }

    struct sockaddr_in adrs;
    memset(&adrs, 0,sizeof(adrs));

    adrs.sin_family = PF_INET;
    adrs.sin_port = htons(PORT);

    if(inet_pton(AF_INET, ADDR,&adrs.sin_addr)!=1){
        perror("inet_pton");
        exit(1);
    }

    if(connect(fd,(struct sockaddr *)&adrs,sizeof(adrs))<0){
        perror("connect");
        exit(1);
    }

    char buf[200];

    int r = recv(fd,buf,199*sizeof(char),0);
    if(r==-1){
        perror("recv");
        exit(1);
    }

    buf[r]='\0';
    printf("%s\n",buf);
}