#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>

// 12121 = recevoir dessus donc on écoute
// 10201 = sur lequel on envoie
#define PORT 10201
#define PORT_ECOUTE 12121
#define SIZE 1024
#define ADDR "ff02::1" // nantucket
#define ADDRNAN "fe80::7e57:58ff:fe68:aea9"

// & nc -6 -u -l 10201 (client qui reçoit, ca marche sur lulu)
// ~ nc -6 -u localhost 12121 (client qui envoie un message, que en local)

int serveur()
{
    //  Faire un diffuseur (serveur)
    //  décalrer socket
    // initialiser l'adresse du multicast (IP + port)
    // récupérer l'index d'une interface locale autorisant le multicast
    // initialiser l'interface local par laquelle partent les multicast.

    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    perror("création de la socket");

    // Activation de SO_REUSEADDR
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt SO_REUSEADDR");
        return 1;
    }

    int index = if_nametoindex("eth0");
    if (index == 0)
    {
        perror("nametoindex");
        return 1;
    }

    struct sockaddr_in6 gradr;
    memset(&gradr, 0, sizeof(gradr));
    gradr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ADDR, &gradr.sin6_addr);
    gradr.sin6_port = htons(PORT);
    gradr.sin6_scope_id = index;

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(PORT_ECOUTE);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return 1;
    }
    while (1)
    {
        char bufrecv[SIZE];
        int total = 0; // Initialisation à 0
        int r;
        while (1)
        {
            r = recv(sock, bufrecv + total, SIZE, 0);
            if (r < 0)
            {
                perror("erreur recv");
                return 1;
            }
            else if (r == 0)
            {
                perror("Connexion fermé");
                bufrecv[r] = '\0';
                break;
            }
            bufrecv[r] = '\0';
            total += r;

            if (strchr(bufrecv, '\n') != NULL)
            {
                break;
            }
        }

        printf("message reçu : %s\n", bufrecv);

        time_t now;
        struct tm *local;
        now = time(NULL);
        local = localtime(&now);
        char time[SIZE];
        sprintf(time, "%02d:%02d", local->tm_hour, local->tm_min);
        char buf[SIZE];
        memset(&buf, 0, SIZE);
        sprintf(buf, "%s\t%s\tGood morning!\n", ADDRNAN, time);

        int s = sendto(sock, buf, SIZE, 0, (struct sockaddr *)&gradr, sizeof(gradr));
        if (s < 0)
        {
            perror("erreur send");
            return 1;
        }
        else
            printf("Message envoyé\n");
    }
    close(sock);
    return 0;
}

int main()
{
    if (serveur() > 0)
    {
        return 1;
    }

    return 0;
}