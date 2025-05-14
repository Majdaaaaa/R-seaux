#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFSIZE 100
#define PORT 4242

int server_1p();
int server_np();

int main(int argn, char **args)
{
    if (argn <= 1 || strcmp(args[1], "-1p") == 0)
    {
        server_1p();
    }
}

/***************LES FONCTIONS DE LECTURE ET ECRITURE SUR SOCKET ******************/

int lire_nb(int sock, char *buf)
{
    int taille, ind = 0;
    do
    {
        if ((taille = recv(sock, buf + ind, BUFSIZE - 1 - ind, 0)) < 0)
        {
            perror("recv");
            exit(1);
        }
        ind += taille;
    } while (ind < BUFSIZE - 1 && taille != 0 && !memchr(buf, '\n', ind));

    if (ind > BUFSIZE)
    {
        fprintf(stderr, "erreur format message recu\n");
        exit(1);
    }
    buf[ind] = 0;

    return ind;
}

int ecrire_nb(int sock, char *buf)
{
    int lg = 0, lu;
    do
    {
        if ((lu = send(sock, buf, strlen(buf), 0)) <= 0)
        {
            perror("send");
            exit(1);
        }
        lg += lu;
    } while (lg < strlen(buf));

    return lg;
}

/***************LES FONCTIONS DE JEU ******************/

/**
 * fait tourner une partie pour un joueur dont le socket est passé en argument
 */
void game_1p(int sock)
{
    srand(time(NULL) + sock);

    // une valeur aléatoire mystere entre 0 et 65536
    // unsigned short int n = rand() % (1 << 16);
    unsigned short int n = rand() % 100;
    printf("nb mystere pour partie socket %d = %d\n", sock, n);

    unsigned short int guess; // le nb proposé par le joueur, sur 2 octets
    int taille = 0;
    int tentatives = 20;
    int gagne = 0;
    char buff_in[BUFSIZE];
    while ((taille = lire_nb(sock, buff_in)) > 0)
    {
        sscanf(buff_in, "%hu", &guess);
        printf("Joueur courant a envoyé : %d\n", guess);
        char reponse[20];
        if (n < guess || n > guess)
        {
            tentatives--;
        }
        if (tentatives == 0)
            sprintf(reponse, "PERDU\n");
        else if (n < guess)
            sprintf(reponse, "MOINS %d\n", tentatives);
        else if (n > guess)
            sprintf(reponse, "PLUS %d\n", tentatives);
        else
        {
            sprintf(reponse, "GAGNE\n");
            gagne = 1;
        }
        ecrire_nb(sock, reponse);
        if (gagne || !tentatives)
            break;
    }
    printf("Fin de partie\n");

    close(sock);
}

int server_1p_fork(int sockclient, int sock)
{
    switch (fork())
    {
    case -1:
        return 1;
    case 0:
        close(sock);
        game_1p(sockclient);
        return 0;
    default:
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
    }
    return 0;
}

void *game_1p_point(void *arg)
{
    int *joueurs = (int *)arg;
    game_1p(*joueurs);
    return NULL;
}

int serveur_1p_thread(int *sockclient, int sock)
{
    pthread_t thread;
    if (pthread_create(&thread, NULL, game_1p_point, sockclient) > 0)
    {
        perror("pthred_create");
        return 1;
    }
    return 0;
}

/**
 * serveur pour jeu 1 player avec 1 connexion à la fois
 */
int server_1p()
{
    int serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int r = bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (r != 0)
    {
        fprintf(stderr, "Échec de bind\n");
        exit(-1);
    }

    r = listen(serv_sock, 0);
    if (r != 0)
    {
        fprintf(stderr, "Échec de listen\n");
        exit(-1);
    }

    while (1)
    {
        int *client_sock = malloc(sizeof(int));
        if (!client_sock)
        {
            perror("malloc");
            exit(-1);
        }

        *client_sock = accept(serv_sock, NULL, NULL);
        if (*client_sock < 0)
        {
            perror("accept");
            free(client_sock);
            continue;
        }

        printf("Connexion acceptée, nouvelle partie lancée.\n");

        pthread_t thread;
        if (pthread_create(&thread, NULL, game_1p_point, client_sock) != 0)
        {
            perror("pthread_create");
            close(*client_sock);
            free(client_sock);
            continue;
        }

        // Détacher le thread pour éviter les fuites de ressources
        pthread_detach(thread);
    }

    close(serv_sock);
    return 0;
}
