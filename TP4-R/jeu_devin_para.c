#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFSIZE 100
#define PORT 4242

// Mutex pour synchronisation
pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER;

// Variables globales
int rang = 0;
int nb_partie = 1;

// Structures pour les arguments des threads
struct arg
{
    int *sockclient;
    int *tentative;
    unsigned short int a_deviner;
};

struct partie_args
{
    int *sockclients; // Tableau des sockets des joueurs
    int nombre_de_joueurs;
    int nombre_de_tentatives;
    unsigned short int a_deviner;
};

// Prototypes des fonctions
int server_np(int, int);
void game_np(int sock, int tent, int);
void *gerer_partie(void *arg);
int lire_nb(int sock, char *buf);
int ecrire_nb(int sock, char *buf);
pthread_t serveur_1p_thread(int *sockclient, struct arg *a);

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
void game_np(int sock, int tent, int a_deviner)
{
    unsigned short int guess;
    int taille = 0, tentatives = tent, gagne = 0;
    char buff_in[BUFSIZE];

    while ((taille = lire_nb(sock, buff_in)) > 0)
    {
        sscanf(buff_in, "%hu", &guess);
        printf("Joueur a envoyé : %d\n", guess);

        char reponse[BUFSIZE];
        pthread_mutex_lock(&verrou);
        if (a_deviner == guess)
        {
            rang++;
            sprintf(reponse, "GAGNE %d\n", rang);
            gagne = 1;
        }
        else
        {
            tentatives--;
            if (tentatives == 0)
            {
                sprintf(reponse, "PERDU\n");
            }
            else if (guess < a_deviner)
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
    game_np(*a->sockclient, *a->tentative, a->a_deviner); // Passez a_deviner
    close(*a->sockclient);
    free(a);
    return NULL;
}

// ==================== GESTION DES PARTIES ====================

// Fonction pour gérer une partie avec plusieurs joueurs
void *gerer_partie(void *arg)
{
    struct partie_args *args = (struct partie_args *)arg;
    int *sockclients = args->sockclients;
    int nombre_de_joueurs = args->nombre_de_joueurs;
    int nombre_de_tentatives = args->nombre_de_tentatives;
    unsigned short int a_deviner = args->a_deviner; // Nombre à deviner local à la partie
    pthread_t threads[nombre_de_joueurs];

    printf("Numéro à deviner pour cette partie : %d\n", a_deviner);

    for (int j = 0; j < nombre_de_joueurs; j++)
    {
        struct arg *a = malloc(sizeof(struct arg));
        if (!a)
        {
            perror("malloc");
            pthread_exit(NULL);
        }
        a->sockclient = &sockclients[j];
        a->tentative = &nombre_de_tentatives;
        a->a_deviner = a_deviner; // Initialisez a_deviner pour ce joueur

        if ((threads[j] = serveur_1p_thread(&sockclients[j], a)) == NULL)
        {
            perror("pthread_create");
            close(sockclients[j]);
            free(a);
            continue;
        }
    }

    for (int j = 0; j < nombre_de_joueurs; j++)
    {
        if (pthread_join(threads[j], NULL) != 0)
        {
            perror("pthread_join");
            pthread_exit(NULL);
        }
        close(sockclients[j]);
    }
    printf("Partie terminée. Tous les joueurs ont terminé.\n");


    free(sockclients);
    free(args);
    pthread_exit(NULL);
}
// ==================== GESTION DES THREADS ====================

// Fonction pour créer un thread pour un joueur
pthread_t serveur_1p_thread(int *sockclient, struct arg *a)
{
    pthread_t thread;
    if (pthread_create(&thread, NULL, game_np_point, a) != 0)
    {
        perror("pthread_create");
        free(a);
        return (pthread_t)NULL;
    }
    return thread;
}

// ==================== SERVEUR ====================

// Fonction principale du serveur
int server_np(int n, int k)
{
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

    while (1)
    {
        int *sockclients = malloc(n * sizeof(int));
        if (!sockclients)
        {
            perror("malloc");
            close(sockserv);
            return 1;
        }

        // Attendre `n` connexions
        for (int i = 0; i < n; i++)
        {
            sockclients[i] = accept(sockserv, NULL, NULL);
            if (sockclients[i] < 0)
            {
                perror("accept");
                free(sockclients);
                close(sockserv);
                return 1;
            }
            printf("Client %d connecté.\n", i + 1);
        }

        // Préparer les arguments pour le thread de la partie
        struct partie_args *args = malloc(sizeof(struct partie_args));
        if (!args)
        {
            perror("malloc");
            free(sockclients);
            close(sockserv);
            return 1;
        }
        args->sockclients = sockclients;
        args->nombre_de_joueurs = n;
        args->nombre_de_tentatives = k;
        args->a_deviner = rand() % 100;

        // Créer un thread pour gérer la partie
        pthread_t thread;
        if (pthread_create(&thread, NULL, gerer_partie, args) != 0)
        {
            perror("pthread_create");
            free(sockclients);
            free(args);
            close(sockserv);
            return 1;
        }
        printf("Partie n°%d démarrée.\n", nb_partie);
        nb_partie++;
        // Détacher le thread pour qu'il puisse se terminer indépendamment
        pthread_detach(thread);
    }

    close(sockserv);
    return 0;
}

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
    return server_np(nombre_de_joueurs, nombre_de_tentatives);
}