#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define PORT 2121
#define ADDR "127.0.0.1"
#define MESS 100

int init_client()
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if ((inet_pton(AF_INET, ADDR, &addr.sin_addr)) != 1)
    {
        perror("conversion échoué");
        return EXIT_FAILURE;
    }

    if ((connect(sock, (struct sockaddr *)&addr, sizeof(&addr))) < 0)
    {
        perror("connect");
        return EXIT_FAILURE;
    }

    printf("Connexion au serveur effectué avec succés! \n");

    return sock;
}

int main()
{
    int s = init_client();

    char buf[MESS];
    int recu;
    int ecrit;

    recu = recv(s, buf, MESS - 1, 0);
    if (recu < 0)
    {
        perror("recv");
        return EXIT_FAILURE;
    }

    buf[recu] = '\0';
    // PLUS r\n\0
    if (buf[recu - 1] == '\n')
    {
        buf[recu - 1] = '\0';
    }
    // PLUS r
    // extraire r
    char *tmp = strchr(buf, ' ');
    if (tmp == NULL)
    {
        printf("mauvais format.\n");
        return EXIT_FAILURE;
    }

    
    buf[recu - strlen(tmp)] = '\0';
    if (strstr(buf, "PLUS") != NULL)
    {
        
    }
    else if (strstr(buf, "MOINS") != NULL)
    {
    }
    else if (strstr(buf, "GAGNE") != NULL)
    {
    }
    else if (strstr(buf, "PERDU") != NULL)
    {
    }
}