#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 9999
#define MAX 30
#define SIZE 512

int serveur()
{
    // socket serveur
    int sock = socket(PF_INET6, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("sock");
        return 1;
    }

    // préparation adresse
    struct sockaddr_in6 adr;
    memset(&adr, 0, sizeof(adr));
    adr.sin6_family = AF_INET6;
    adr.sin6_addr = in6addr_any;
    adr.sin6_port = htons(PORT);

    // bind
    if (bind(sock, (struct sockaddr *)&adr, sizeof(adr)) < 0)
    {
        perror("bind");
        close(sock);
        return 1;
    }

    // listen
    if (listen(sock, 0) < 0)
    {
        perror("listen");
        close(sock);
        return 1;
    }

    printf("Serveur démarré sur le port %d...\n", PORT);

    // Init des ensembles
    fd_set lecture, lcpy;
    fd_set ecriture, ecpy; // On a pas besoin ici de le surveiller en écriture
    // max_fd pour select
    int max_fd;
    // tableau des socket clients
    int client_sockets[MAX];
    // donné recu par le serveur
    char buffer[SIZE];
    // Donner a envoyé par clients (un buffer par client)
    char data_to_send[MAX][SIZE];
    // Taille des données à envoyer par clients de base
    int data_len[MAX];

    // Initialiser tous les sockets client à 0
    for (int i = 0; i < MAX; i++)
    {
        client_sockets[i] = 0;
        data_len[i] = 0; // taille des données envoyé
    }

    // INIT des ensembles

    // * Aurait pu ne pas faire ça fallait mieux lire mais vasy ca marche 
    FD_ZERO(&lecture);
    FD_ZERO(&ecriture);
    // Pas besoin de surveiller sock en écriture psq cette socket fait pas connect, write, send ou sendto
    FD_SET(sock, &lecture);
    max_fd = sock;

    while (1)
    {
        // Copie des ensembles car vont être modifié par select
        lcpy = lecture;
        ecpy = ecriture;

        int act = select(max_fd + 1, &lcpy, &ecpy, NULL, NULL);

        if (act < 0)
        {
            perror("select");
            break;
        }

        // La socket sock serveur est prête en lecture (donc peut faire un accept)
        if (FD_ISSET(sock, &lcpy))
        {
            int new_socket = accept(sock, NULL, NULL);
            if (new_socket < 0)
            {
                perror("accept");
            }
            else
            {
                printf("Nouvelle connexion, socket fd: %d\n", new_socket);

                // Ajouter le nouveau socket au tableau et aux ensembles à surveiller
                int added = 0;
                // Boucle pour trouver le 1er emplacement vide
                for (int i = 0; i < MAX; i++)
                {
                    if (client_sockets[i] == 0)
                    {
                        // ajout de la socket au tableau de
                        client_sockets[i] = new_socket;
                        FD_SET(new_socket, &lecture); // Surveiller en lecture
                        // la surveillane en écriture viens apres

                        // Mettre à jour max_fd si nécessaire
                        if (new_socket > max_fd)
                        {
                            max_fd = new_socket;
                        }

                        printf("Client ajouté à l'indice %d\n", i);
                        added = 1;
                        break;
                    }
                }

                if (!added)
                {
                    printf("Impossible d'ajouter plus de clients, connexion refusée\n");
                    close(new_socket);
                }
            }
        }

        // Vérifier les sockets clients pour des données à lire
        for (int i = 0; i < MAX; i++)
        {
            int sd = client_sockets[i];

            // si la socket existe et qu'elle est prete en lecture :
            if (sd > 0 && FD_ISSET(sd, &lcpy))
            {
                // Lire les données du client
                memset(buffer, 0, SIZE);
                int bytes_read = 0;
                // Boucle du recv
                while (1)
                {
                    int r = recv(sd, buffer + bytes_read, SIZE - bytes_read - 1, 0);
                    if (r < 0)
                    {
                        fprintf(stderr, "Erreur dans le recv du client %d..\n", sd);
                        close(sd);
                        client_sockets[i] = 0;
                        FD_CLR(sd, &lecture);
                        FD_CLR(sd, &ecriture);
                        data_len[i] = 0;
                        break;
                    }
                    else if (r == 0)
                    {
                        printf("Client déconnecté, socket fd: %d\n", sd);
                        close(sd);
                        client_sockets[i] = 0;
                        FD_CLR(sd, &lecture);
                        FD_CLR(sd, &ecriture);
                        data_len[i] = 0;
                        break;
                    }
                    else
                    {
                        bytes_read += r;
                        buffer[bytes_read] = '\0';
                        if (strstr(buffer, "\n") != NULL)
                        {
                            printf("Fin de la lecture pour le client %d.\n", sd);
                            printf("Message reçu du client %d: %s", sd, buffer);

                            // Copier dans le buffer à envoyer
                            memcpy(data_to_send[i], buffer, bytes_read);
                            data_len[i] = bytes_read;

                            // Ajouter le socket à l'ensemble d'écriture pour envoyer la réponse
                            FD_SET(sd, &ecriture);
                            break;
                        }
                    }
                }
            }

            // Vérifier si on peut écrire des données (écho) au client
            // si la socket existe, que elle est prête en écriture et que ya des données à envoyer
            if (sd > 0 && FD_ISSET(sd, &ecpy) && data_len[i] > 0)
            {
                // Envoyer les données en écho
                int bytes_sent = 0;
                while (1)
                {
                    int s = send(sd, data_to_send[i] + bytes_sent, data_len[i] - bytes_sent, 0);

                    if (s <= 0)
                    {
                        perror("send");
                    }
                    bytes_sent += s;
                    if (bytes_sent >= data_len[i])
                    {
                        printf("Send fini pour le client %d.\n", sd);
                        // Réinitialiser les données à envoyer
                        data_len[i] = 0;

                        // Ne plus surveiller en écriture jusqu'à ce qu'on reçoive de nouvelles données
                        FD_CLR(sd, &ecriture);
                        break;
                    }
                }
            }
        }
    }

    close(sock);
    return 0;
}

int main()
{
    return serveur();
}