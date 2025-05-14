#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SIZE_MSG 1024
#define ADDR "lampe"
#define port "2628"

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

void affiche_connexion(struct addrinfo *p)
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
}

int main(int argc, char *argv[])
{
    char *mot;
    if (argc > 1)
    {
        mot = argv[1];
    }
    else
    {
        fprintf(stdout, "usage : ./a.out <mot>\n");
        exit(1);
    }
    int socket_fd;

    struct addrinfo hints, *r, *p;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_V4MAPPED | AI_ALL;
    hints.ai_protocol = 0;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ADDR, port, &hints, &r) > 0)
    {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    p = r;
    while (p != NULL)
    {
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) > 0)
        {
            if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == 0)
            {
                printf("Connexion réussie à : ");
                affiche_connexion(p);
                break;
            }
            printf("Connexion échouée à l'adresse : ");
            affiche_connexion(p);
            close(socket_fd);
        }
        p = p->ai_next;
    }

    if (p == NULL)
        exit(EXIT_FAILURE);

    int recu = 0;
    char buf[SIZE_MSG];
    while (recu < 4)
    {
        recu += recv(socket_fd, buf + recu, SIZE_MSG, 0);
        if (recu <= 0)
        {
            perror("recv");
            exit(EXIT_FAILURE);
        }
    }
    buf[recu] = '\0';

    if (strncmp(buf, "220 ", 4) != 0)
    {
        printf("La réponse du serveur ne commence pas par '220 '.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("La réponse du serveur commence par '220 '.\n");
    }

    int s=0;
    char bufsend[SIZE_MSG];

    sprintf(bufsend, "DEFINE * ");
    snprintf(bufsend + strlen("DEFINE * "), SIZE_MSG - strlen("DEFINE * "), "%s\r\n", mot);
    printf("a envoyé : %s\n",bufsend);
    while (s < strlen(bufsend))
    {
        s += send(socket_fd, bufsend + s, strlen(bufsend) - s, 0);
        if (s <= 0)
        {
            perror("send");
            exit(EXIT_FAILURE);
        }
    }

    recu = 0;
    while (1)
    {
        int bytes_received = recv(socket_fd, buf + recu, SIZE_MSG - recu, 0);
        if (bytes_received <= 0)
        {
            perror("recv");
            exit(EXIT_FAILURE);
        }
        recu += bytes_received;
        buf[recu] = '\0';
    
        // Vérifiez si la réponse est complète (par exemple, si elle se termine par "\r\n.\r\n")
        if (strstr(buf, "\r\n.\r\n") != NULL)
        {
            break;
        }
        if(recu==SIZE_MSG){
            recu=0;
            printf("\n%s\n", buf);
        }
    }
	 printf("\n%s\n", buf);    


    close(socket_fd);
    freeaddrinfo(r);
    return 0;
}
