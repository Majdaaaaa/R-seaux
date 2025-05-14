#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

union adresse
{
    char adr4[INET_ADDRSTRLEN];
    char adr6[INET6_ADDRSTRLEN];
};

union sockadresse
{
    struct sockaddr_in sadr4;
    struct sockaddr_in6 sadr6;
};

void affiche_adresse(union sockadresse addr, int adrlen)
{
    union adresse buf;

    if (adrlen == sizeof(struct sockaddr_in))
    {
        inet_ntop(AF_INET, &(addr.sadr4.sin_addr), buf.adr4, sizeof(buf.adr4));
        printf("adresse serveur : IP: %s port: %d\n", buf.adr4, ntohs(addr.sadr4.sin_port));
    }
    else
    {
        inet_ntop(AF_INET6, &(addr.sadr6.sin6_addr), buf.adr6, sizeof(buf.adr6));
        printf("adresse serveur : IP: %s port: %d\n", buf.adr6, ntohs(addr.sadr6.sin6_port));
    }
}

void getAddrFromName(char *name, char *port)
{
    struct addrinfo hints, *r, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = 0;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    if (getaddrinfo(name, port, &hints, &r) > 0)
    {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    p = r;
    while (p != NULL)
    {
        union adresse buf;
        if (p->ai_addrlen == sizeof(struct sockaddr_in))
        {
            inet_ntop(AF_INET, &(((struct sockaddr_in *)p->ai_addr)->sin_addr), buf.adr4, sizeof(buf.adr4));
            printf("adresse serveur : IP: %s port: %d\n", buf.adr4, ntohs(((struct sockaddr_in *)p->ai_addr)->sin_port));
        }
        else
        {
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)p->ai_addr)->sin6_addr), buf.adr6, sizeof(buf.adr6));
            printf("adresse serveur : IP: %s port: %d\n", buf.adr6, ntohs(((struct sockaddr_in6 *)p->ai_addr)->sin6_port));
        }

        p = p->ai_next;
    }
    if (p == NULL)
        exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    char *addr;
    char *port;
    if (argc > 1)
    {
        addr = argv[1];
        port = argv[2];
    }

    getAddrFromName(addr, port);
}
