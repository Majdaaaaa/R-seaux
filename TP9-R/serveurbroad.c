#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>

#define ADDR "192.168.70.255"
#define SIZE 1024
#define PORT 9999

int serveur()
{
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    // * RECEPTION
    struct sockaddr_in address_sock;
    address_sock.sin_family = AF_INET;
    address_sock.sin_port = htons(PORT);
    address_sock.sin_addr.s_addr = htonl(INADDR_ANY);

    int r = bind(sock, (struct sockaddr *)&address_sock, sizeof(struct sockaddr_in));
    if (r)
    {
        perror("bind");
        close(sock);
        return 1;
    }

    struct sockaddr_in emet;
    socklen_t taille = sizeof(emet);
    char buf[SIZE], adr[100];
    int rec = recvfrom(sock, buf, 100, 0, (struct sockaddr *)&emet, &taille);
    buf[rec] = '\0';
    printf("Message recu : %s\n", buf);

    // * ENVOIE
    int ok = 1;
    r = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &ok, sizeof(ok));
    if (r == -1)
    {
        perror("setsockopt");
        close(sock);
        return 1;
    }

    struct sockaddr_in adrdiff;
    memset(&adrdiff, 0, sizeof(adrdiff));
    adrdiff.sin_family = AF_INET;
    adrdiff.sin_port = htons(PORT);
    r = inet_pton(AF_INET, ADDR, &adrdiff.sin_addr);
    if (r <= 0)
    {
        perror("inet_pton");
        close(sock);
        return 1;
    }
    char rbuf[SIZE];
    sprintf(rbuf, "ACK\n");
    r = sendto(sock, rbuf, strlen(rbuf), 0, (struct sockaddr *)&adrdiff,
               (socklen_t)sizeof(struct sockaddr_in));
    if (r < 0)
    {
        perror("sendto");
        close(sock);
        return 1;
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