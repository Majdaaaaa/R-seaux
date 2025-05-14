#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>

#define PORT 10201
#define SIZE 1500

// ~ Faire un abonné :
// * déclarer la socket
// * init l'adresse de réception (le port sur lequel on veut recevoir les données)
// * lier la socket à l'adresse (bind)
// * récupérer l'index de l'interface (pareil que le serveur)
// * init l'adresse de l'abonnement (l'adresse IPv6 du groupe + l'interface)
// *  s'abonner au groupe en modifiant les options de la socket

// & dans cet exo
// Le client lit l'entrée standard et envoie ce qu'il lit au serveur
// en même temps il lit les messages qu'il reçoit de la part du serveur sur le port 10201 et les affiche sur la sortie standard

int client()
{

    // * déclarer la socket
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("création de la socket");
        return 1;
    }

    // * init l'adresse de réception (le port sur lequel on veut recevoir les données)
    struct sockaddr_in6 adr;
    memset(&adr, 0, sizeof(adr));
    adr.sin6_family = AF_INET6;
    adr.sin6_addr = in6addr_any;
    adr.sin6_port = htons(PORT);

    // * lier la socket à l'adresse (bind)
    if (bind(sock, (struct sockaddr *)&adr, sizeof(adr)))
    {
        perror("erreur bind");
        close(sock);
        return 1;
    }

    // * récupérer l'index de l'interface (pareil que le serveur)
    int index = if_nametoindex("eth0");
    if (index == 0)
    {
        perror("nametoindex");
        close(sock);
        return 1;
    }

    // * init l'adresse de l'abonnement (l'adresse IPv6 du groupe + l'interface)
    struct ipv6_mreq group;
    inet_pton(AF_INET6, "ff02::1", &group.ipv6mr_multiaddr);
    group.ipv6mr_interface = index;

    // * s'abonner au groupe en modifiant les options de la socket
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group, sizeof(group)) < 0)
    {
        perror("erreur abonnement groupe");
        close(sock);
        return 1;
    }

    // ? permet de réutiliser un port sans attente
    int yes = 1;
    int r = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (r < 0)
    {
        perror("setsockopt reuseaddr");
        close(sock);
        return 1;
    }

    printf("Entrez le message à envoyer : \n");
    char buf[SIZE];
    int rin = read(STDIN_FILENO, buf, SIZE);
    if (rin < 0)
    {
        perror("read stdin");
        close(sock);
        return 1;
    }
    printf("message à envoyer : %s\n", buf);

    struct sockaddr_in6 sendaddr;
    memset(&sendaddr, 0, sizeof(sendaddr));
    sendaddr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "fdc7:9dd5:2c66:be86:7e57:58ff:fe68:aea9", &sendaddr.sin6_addr);
    sendaddr.sin6_port = htons(12121);

    int s = sendto(sock, buf, strlen(buf), 0, (struct sockaddr *)&sendaddr, sizeof(sendaddr));
    if (s < 0)
    {
        perror("sendto");
        close(sock);
        return 1;
    }

    char recvbuf[SIZE];
    while (1)
    {
        int rsock = recv(sock, recvbuf, SIZE - 1, 0);
        if (rsock < 0)
        {
            perror("recv sock");
            close(sock);
            return 1;
        }
        // Assurer la terminaison de la chaîne
        recvbuf[rsock] = '\0';
        printf("Message reçu : %s", recvbuf);
        // Si on trouve un '\n' dans le message, on sort de la boucle.
        if (strchr(recvbuf, '\n') != NULL)
        {
            break;
        }
    }

    return 0;
}

int main()
{
    if (client() > 0)
    {
        return 1;
    }
    return 0;
}