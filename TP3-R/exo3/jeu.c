#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define TENTATIVE 20
#define PORT 2121
#define MESS 100

// retourne le descriteur de socket du serveur
int initServ()
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("créeation de la socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((bind(sock, (struct sockaddr *)&addr, sizeof(addr))) < 0)
    {
        perror("bind");
        close(sock);
        return EXIT_FAILURE;
    }

    if ((listen(sock, 5)) < 0)
    {
        perror("listen");
        close(sock);
        return EXIT_FAILURE;
    }

    printf("Serveur TCP Echo en attente de connexions sur le port %d...\n", PORT);

    return sock;
}


int main()
{
    unsigned int seed;
    FILE *urandom = fopen("/dev/urandom", "r");
    fread(&seed, sizeof(seed), 1, urandom);
    fclose(urandom);
    srand(seed);
    char buf[MESS];
    int r = rand() % 65535;
    int sock = initServ();
    if (sock == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }
    int sock_client = accept(sock, NULL, NULL);
    sprintf(buf, "Client connecté.\n Bienvenue dans le jeu.\n Vous avez %d tentatives pour trouver le nombre caché.\n", TENTATIVE);
    int s = send(sock_client, buf, strlen(buf),0);
    if (s <= 0)
    {
        perror("erreur d'écriture");
        return 1;
    }
    int tent_client = 0;

    while (tent_client < TENTATIVE)
    {
        memset(buf, 0, MESS);
        int re = recv(sock_client, buf, MESS - 1, 0);
        if (re < 0)
        {
            perror("erreur de lecture");
            break;
        }
        else if (re == 0)
        {
            printf("Connexion fermée par le client.\n");
            break; // Sortir de la boucle si la connexion est fermée
        }
        buf[re] = '\0';
        tent_client++;
        // vérfier le format
        if (buf[re - 1] == '\n')
        {
            buf[re - 1] = '\0';
            int reponse = atoi(buf);
            printf("reponse : %d\n", reponse);
            if (tent_client > 0)
            {
                if (reponse < r)
                {
                    sprintf(buf, "PLUS %d\n", TENTATIVE - tent_client);
                    s = send(sock_client, buf, strlen(buf), 0);
                    if (s <= 0)
                    {
                        perror("erreur d'écriture");
                        break;
                    }
                }
                else if (reponse > r)
                {
                    sprintf(buf, "MOINS %d\n", TENTATIVE - tent_client);
                    s = send(sock_client, buf, strlen(buf), 0);
                    if (s <= 0)
                    {
                        perror("erreur d'écriture");
                        break;
                    }
                }
                else if (reponse == r)
                {
                    sprintf(buf, "GAGNE\n");
                    s = send(sock_client, buf, strlen(buf), 0);
                    if (s <= 0)
                    {
                        perror("erreur d'écriture");
                        break;
                    }
                    printf("FIN DU JEU. MERCI.\n");
                    break;
                }
            }
        }
        else
        {
            printf("Format pas respecté : r\\n\n");
            continue;
        }
    }

    sprintf(buf, "PERDU\nLa réponse était : %d\n", r);

    s = send(sock_client, buf, strlen(buf), 0);
    if (s <= 0)
    {
        perror("erreur d'écriture");
        close(sock);
        close(sock_client);
        return EXIT_SUCCESS;
    }
    printf("FIN DU JEU. MERCI.\n");

    close(sock);
    close(sock_client);
    return EXIT_SUCCESS;
}