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
#define ADDR "224.0.0.1" // nantucket multicast
#define ADDRNAN "192.168.70.100"

int serveur4()
{
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return 1;

    /* Initialisation de l’adresse d’abonnement */
    struct sockaddr_in gradr;
    memset(&gradr, 0, sizeof(gradr));
    gradr.sin_family = AF_INET;
    inet_pton(AF_INET, ADDR, &gradr.sin_addr);
    gradr.sin_port = htons(PORT);

    /* Initialisation de l’interface */
    int ifindex = if_nametoindex("eth0");

    struct ip_mreqn group;
    memset(&group, 0, sizeof(group));
    group.imr_multiaddr.s_addr = htonl(INADDR_ANY);
    group.imr_ifindex = ifindex;

    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &group, sizeof(group)))
    {
        perror("erreur initialisation de l’interface locale");
        return 1;
    }

    /* Initialisation de l’interface par défaut */
    int ok = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &ok, sizeof(ok)))
    {
        perror("erreur initialisation de l’interface locale");
        return 1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT_ECOUTE);

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
    if (serveur4() > 0)
    {
        return 1;
    }

    return 0;
}