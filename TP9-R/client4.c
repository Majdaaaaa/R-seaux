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
#define ADDR "224.0.0.1" // nantucket multicast

int client4()
{
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return 1;

    struct sockaddr_in adr;
    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr = htonl(INADDR_ANY);
    adr.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&adr, sizeof(adr)))
    {
        perror("erreur bind");
        close(sock);
        return 1;
    }

    struct ip_mreqn group;
    memset(&group, 0, sizeof(group));
    inet_pton(AF_INET, ADDR, &group.imr_multiaddr);
    group.imr_ifindex = if_nametoindex("eth0");

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group)) < 0)
    {
        perror("echec de abonnement groupe");
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

    struct sockaddr_in sendaddr;
    memset(&sendaddr, 0, sizeof(sendaddr));
    sendaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.70.100", &sendaddr.sin_addr);
    sendaddr.sin_port = htons(12121);

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
    if (client4() > 0)
    {
        return 1;
    }
    return 0;
}