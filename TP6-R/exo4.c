#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX 1024
/*
Dans cet exercice vous allez écrire un client IPv6 polymorphe qui essaie de se
connecter à dict. On ne sait pas à l’avance l’adresse de la machine,
par conséquent, on ne sait pas si le serveur est en IPv4 ou en IPv6.

1. Étant donné que l’on connaît le nom de la machine à connecter, utilisez
getaddrinfo pour récupérer son adresse. Faites attention : nous voulons que le
client soit en IPv6, donc il faut choisir une bonne valeur
pour hints.ai_family, et qu’il soit polymorphe, donc il faut choisir une bonne
valeur pour hints.ai_flags
*/

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        perror("pas assez d'arguments");
        exit(EXIT_FAILURE);
    }
    char *mot = argv[1];
    struct addrinfo hints, *r, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // famille de la socket sera quoi on sait pas encore si on sera en IPV6 ou IPV4
    hints.ai_flags = AI_V4MAPPED | AI_ALL;
    hints.ai_protocol = 0;
    hints.ai_socktype = SOCK_STREAM;
    if ((getaddrinfo("lampe", "2628", &hints, &r)) > 0)
    {
        exit(EXIT_FAILURE);
    } // Q1
    char buf[MAX];
    p = r;
    int sock;
    while (p != NULL)
    {
        if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) > 0)
        {
            if (connect(sock, p->ai_addr, p->ai_addrlen) == 0)
            {
                perror("connexion ok");
                break;
            }
        }
        p = p->ai_next;
    }
    if (p == NULL)
    {
        printf("connexion pas abouttie\n");
        exit(EXIT_FAILURE);
    }
    printf("Connexion réussie\n");
    int ree = 0;
    while (ree < 4)
    {
        ree += recv(sock, buf + ree, MAX, 0);
        if (ree <= 0)
        {
            perror("erreur dans le recv");
            exit(EXIT_FAILURE);
        }
    }
    buf[ree] = '\0';

    if (strncmp(buf, "220 ", 4) != 0)
    {
        printf("On a pas reçu %d\n", 220);
        exit(EXIT_FAILURE);
    }
    printf("On a bien reçu %d\n", 220);

    char buf_send[MAX];
    snprintf(buf_send, MAX, "DEFINE * %s\r\n", mot);
    printf("Le mot qu'on va envoyé : %s\n", mot);
    int s = 0;
    while (s < strlen(buf_send))
    {
        s += send(sock, buf_send + s, strlen(buf_send) - s, 0);
        if (s <= 0)
        {
            perror("send");
            exit(EXIT_FAILURE);
        }
    }
    printf("message envoyé avec succes\n");

    char rep[MAX];
    memset(rep, 0, MAX);
    int recu = 0;
    while (1)
    {
        perror("dans la boucle");
        int re = recv(sock, rep + recu, MAX - recu, 0);
        if (re <= 0)
        {
            perror("problème dans le recv");
            exit(EXIT_FAILURE);
        }
        recu += re;
        rep[recu] = '\0';

        if (strstr(rep, "\r\n.\r\n") != NULL)
        {
            perror("fin trouvé");
            break;
        }

        if (recu == MAX)
        {
            printf("Le message du serveur \n%s\n", rep);
            // memset(rep, 0, MAX);
            recu = 0;
        }
    }
    printf("%s\n", rep);
    close(sock);
    freeaddrinfo(r);
    exit(EXIT_SUCCESS);
}

// Q2