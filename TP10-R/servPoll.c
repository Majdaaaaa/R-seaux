#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <poll.h>

#define PORT 9999
#define MAX 5
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

    int listener = sock;
    int sockclient, fd_count=0;
    char buffer[SIZE];
    char buff_client[MAX][SIZE];
    int data_len[MAX];
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX);
    memset(pfds, 0, sizeof(*pfds) * MAX);
    pfds[0].fd = listener;   // sock à surveiller
    pfds[0].events = POLLIN; // attente en lecture
    fd_count++;

    while (1)
    {
        // 1000 = temps à attendre (timeout)
        int p = poll(pfds, fd_count, 1000);

        if (p == -1)
        {
            perror("poll");
            close(sock);
            return 1;
        }

        if (p == 0)
        {
            printf("Il se passe rien et le temps est écoulé\n");
            continue;
        }

        for (int i = 0; i < MAX; i++)
        {
            // une socket est dispo en lecture
            if (pfds[i].revents & POLLIN)
            {
                // Si c'est la socket serveur on accepte:
                if (pfds[i].fd == listener)
                {
                    if (fd_count >= MAX) // on pas encore max clients donc on peut continuer d'accepter
                        continue;
                    sockclient = accept(listener, NULL, NULL);

                    if (sockclient == -1)
                    {
                        perror("accept");
                        close(sock);
                        return 1;
                    }
                    else
                    {
                        // ajouter la surveillance de la socket client en lecture
                        pfds[fd_count].fd = sockclient;
                        pfds[fd_count].events = POLLIN;

                        fd_count++;
                        printf("Nouvelle connexion du client : %d .\n", sockclient);
                    }
                }
                else // si c'est une socket client :
                {
                    int bytes_recv = 0;
                    while (1)
                    {
                        int r = recv(pfds[i].fd, buffer + bytes_recv, SIZE - bytes_recv - 1, 0);
                        if (r < 0)
                        {
                            fprintf(stderr, "Erreur dans le recv du client %d..\n", pfds[i].fd);
                            close(pfds[i].fd);
                            // Réorganiser le tableau pfds pour supprimer cette connexion
                            for (int j = i; j < fd_count - 1; j++)
                            {
                                pfds[j] = pfds[j + 1];
                                data_len[j] = data_len[j + 1];
                                memcpy(buff_client[j], buff_client[j + 1], SIZE);
                            }
                            fd_count--;
                            i--; // Pour ne pas sauter le prochain élément du tableau
                            break;
                        }
                        else if (r == 0)
                        {
                            printf("Client déconnecté, socket fd: %d\n", pfds[i].fd);
                            close(pfds[i].fd);
                            // Réorganiser le tableau pfds pour supprimer cette connexion
                            for (int j = i; j < fd_count - 1; j++)
                            {
                                pfds[j] = pfds[j + 1];
                                data_len[j] = data_len[j + 1];
                                memcpy(buff_client[j], buff_client[j + 1], SIZE);
                            }
                            fd_count--;
                            i--; // Pour ne pas sauter le prochain élément du tableau
                            break;
                        }
                        else
                        {
                            bytes_recv += r;
                            buffer[bytes_recv] = '\0';
                            if (strstr(buffer, "\n") != NULL)
                            {
                                printf("Fin de la lecture pour le client %d.\n", pfds[i].fd);
                                printf("Message reçu du client %d: %s", pfds[i].fd, buffer);

                                // Copier dans le buffer à envoyer
                                memcpy(buff_client[i], buffer, bytes_recv + 1); // +1 pour inclure le caractère nul
                                data_len[i] = bytes_recv;

                                pfds[i].events = POLLOUT;
                                break;
                            }
                        }
                    }
                }
            }
            else if (pfds[i].revents & POLLOUT)
            {
                // Envoyer les données en écho
                int bytes_sent = 0;
                while (bytes_sent < data_len[i])
                {
                    int s = send(pfds[i].fd, buff_client[i] + bytes_sent, data_len[i] - bytes_sent, 0);

                    if (s <= 0)
                    {
                        perror("send");
                        break;
                    }
                    bytes_sent += s;
                }

                printf("Send fini pour le client %d.\n", pfds[i].fd);
                // Réinitialiser les données à envoyer
                data_len[i] = 0;

                // Revenir à la surveillance en lecture
                pfds[i].events = POLLIN;
            }
        }
    }
    free(pfds);
    close(sock);
    return 0;
}

int main()
{
    return serveur();
}