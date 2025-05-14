#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFSIZE 100
#define PORT 4242

// se fait en variable globle psq sinon pas tout les threads pourront y
// avoir accés si c'est dans une fonction
// détruit a la fin de la fonction

// & Ceci est un serveur, utiliser netcat pour crée un client avec nc localhost 4242

// ? version IPv6
// ** version hybride
/**
 * ? struct sockaddr_in6 addrserv;
 * ? addrserv.sin6_addr = in6addr_any;
 * ? addrserv.sin6_port = htons(PORT);
 * ? addrserv.sin_family = AF_INET6;
 */

/* Pour désactiver l'option qui accepte que les adresses IPv6
 * int optval = 0;
 * int r = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval));
 * if (r < 0)
 * perror("erreur connexion IPv4 impossible");
 */
// ==================== VARIABLES GLOBALES ====================
pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER;
unsigned short int a_deviner;
int rang = 1;

// Structure pour passer des arguments aux threads
struct arg
{
    int *sockclient;
    int *tentative;
};

// ==================== PROTOTYPES DES FONCTIONS ====================
int server_np(int n, int k);
int lire_nb(int sock, char *buf);
int ecrire_nb(int sock, char *buf);
void game_np(int sock, int tent);
void *game_np_point(void *arg);
pthread_t serveur_1p_thread(int *sockclient, struct arg *a);

// ==================== MAIN ====================
int main(int argn, char **args)
{
    if (argn != 4 || strcmp(args[1], "-np") != 0)
    {
        fprintf(stderr, "Usage: %s -np <nombre_de_joueurs> <nombre_de_tentatives>\n", args[0]);
        return 1;
    }

    int nombre_de_joueurs = atoi(args[2]);
    int nombre_de_tentatives = atoi(args[3]);

    if (nombre_de_joueurs <= 0 || nombre_de_tentatives <= 0)
    {
        fprintf(stderr, "Le nombre de joueurs et le nombre de tentatives doivent être des entiers positifs.\n");
        return 1;
    }

    srand(time(NULL));
    pthread_mutex_lock(&verrou);
    a_deviner = rand() % 100;
    pthread_mutex_unlock(&verrou);
    printf("Nombre mystère = %d\n", a_deviner);

    return server_np(nombre_de_joueurs, nombre_de_tentatives);
}

// ==================== FONCTIONS UTILITAIRES ====================

// Fonction pour lire un message depuis un socket
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
        fprintf(stderr, "Erreur : message reçu trop long\n");
        exit(1);
    }
    buf[ind] = 0;
    return ind;
}

// Fonction pour écrire un message sur un socket
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

// ==================== LOGIQUE DU JEU ====================

// Fonction pour gérer une partie pour un joueur
void game_np(int sock, int tent)
{
    unsigned short int guess;
    int tentatives = tent;
    int gagne = 0;
    char buff_in[BUFSIZE];

    while (lire_nb(sock, buff_in) > 0)
    {
        sscanf(buff_in, "%hu", &guess);
        printf("Joueur a envoyé : %d\n", guess);

        char reponse[BUFSIZE];
        pthread_mutex_lock(&verrou);
        if (a_deviner == guess)
        {
            sprintf(reponse, "GAGNE %d\n", rang);
            rang++;
            gagne = 1;
        }
        else
        {
            tentatives--;
            if (tentatives == 0)
            {
                sprintf(reponse, "PERDU\n");
            }
            else if (a_deviner < guess)
            {
                sprintf(reponse, "PLUS %d\n", tentatives);
            }
            else
            {
                sprintf(reponse, "MOINS %d\n", tentatives);
            }
        }
        pthread_mutex_unlock(&verrou);

        ecrire_nb(sock, reponse);
        if (gagne || tentatives == 0)
            break;
    }
    printf("Fin de partie pour un joueur.\n");
    close(sock);
}

// Fonction pour exécuter `game_np` dans un thread
void *game_np_point(void *arg)
{
    struct arg *a = (struct arg *)arg;
    game_np(*a->sockclient, *a->tentative);
    close(*a->sockclient);
    free(a);
    return NULL;
}

// ==================== GESTION DES THREADS ====================

// Fonction pour créer un thread pour un joueur
pthread_t serveur_1p_thread(int *sockclient, struct arg *a)
{
    pthread_t thread;
    if (pthread_create(&thread, NULL, game_np_point, a) != 0)
    {
        perror("pthread_create");
        free(sockclient);
        return (pthread_t)NULL;
    }
    return thread;
}

// ==================== SERVEUR ====================

// Fonction principale du serveur
int server_np(int n, int k)
{
    int *sockclient[n];
    pthread_t threads[n];

    int sockserv = socket(PF_INET, SOCK_STREAM, 0);
    if (sockserv < 0)
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addrserv;
    memset(&addrserv, 0, sizeof(addrserv));
    addrserv.sin_addr.s_addr = htonl(INADDR_ANY);
    addrserv.sin_port = htons(PORT);
    addrserv.sin_family = AF_INET;

    if (bind(sockserv, (struct sockaddr *)&addrserv, sizeof(addrserv)) < 0)
    {
        perror("bind");
        close(sockserv);
        return 1;
    }

    if (listen(sockserv, n) < 0)
    {
        perror("listen");
        close(sockserv);
        return 1;
    }

    printf("Serveur en attente de connexions...\n");

    int i = 0;
    while (i < n)
    {
        int sock = accept(sockserv, NULL, NULL);
        if (sock < 0)
        {
            perror("accept");
            continue;
        }
        printf("Client %d connecté.\n", i + 1);
        sockclient[i] = malloc(sizeof(int));
        *sockclient[i] = sock;
        i++;
    }

    printf("Les %d clients sont connectés.\n", n);

    for (int j = 0; j < n; j++)
    {
        struct arg *a = malloc(sizeof(struct arg));
        if (!a)
        {
            perror("malloc");
            close(sockserv);
            return 1;
        }
        a->sockclient = sockclient[j];
        a->tentative = &k;

        if ((threads[j] = serveur_1p_thread(sockclient[j], a)) == NULL)
        {
            perror("pthread_create");
            close(sockserv);
            close(*sockclient[j]);
            free(a);
            continue;
        }
    }

    for (int j = 0; j < n; j++)
    {
        if (pthread_join(threads[j], NULL) != 0)
        {
            perror("pthread_join");
            close(sockserv);
            return 1;
        }
        close(*sockclient[j]);
        free(sockclient[j]);
    }

    close(sockserv);
    return 0;
}