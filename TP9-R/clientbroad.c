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

// Fonction de configuration et de binding de la socket
int setupSocket()
{
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return -1;
    }

    // Activation de l'option broadcast
    int ok = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &ok, sizeof(ok)) == -1)
    {
        perror("setsockopt");
        close(sock);
        return -1;
    }

    // Binding de la socket au port pour la réception
    struct sockaddr_in address_sock;
    address_sock.sin_family = AF_INET;
    address_sock.sin_port = htons(PORT);
    address_sock.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&address_sock, sizeof(address_sock)) != 0)
    {
        perror("bind");
        close(sock);
        return -1;
    }
    return sock;
}

// Fonction d'envoi du message broadcast
int sendBroadcast(int sock)
{
    struct sockaddr_in adrdiff;
    memset(&adrdiff, 0, sizeof(adrdiff));
    adrdiff.sin_family = AF_INET;
    adrdiff.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ADDR, &adrdiff.sin_addr) <= 0)
    {
        perror("inet_pton");
        return -1;
    }

    char buf[SIZE];
    sprintf(buf, "HELLO\n");
    int r = sendto(sock, buf, strlen(buf), 0, (struct sockaddr *)&adrdiff, sizeof(struct sockaddr_in));
    if (r < 0)
    {
        perror("sendto");
        return -1;
    }
    printf("Message envoyé.\n");
    return 0;
}

// Fonction de réception des messages
void receiveMessages(int sock)
{
    struct sockaddr_in emet;
    socklen_t taille = sizeof(emet);
    char buf[SIZE];
    char adr[100];

    while (1)
    {
        int rec = recvfrom(sock, buf, SIZE - 1, 0, (struct sockaddr *)&emet, &taille);
        if (rec < 0)
        {
            perror("recvfrom");
            break;
        }
        buf[rec] = '\0';
        printf("Message reçu : %s\n", buf);
        printf("Port de l'émetteur : %d\n", ntohs(emet.sin_port));
        inet_ntop(AF_INET, &(emet.sin_addr), adr, sizeof(adr));
        printf("Le serveur tourne sur l'adresse : %s\n\n", adr);
    }
}

int client()
{
    int sock = setupSocket();
    if (sock < 0)
        return 1;

    if (sendBroadcast(sock) < 0)
    {
        close(sock);
        return 1;
    }

    receiveMessages(sock);

    close(sock);
    return 0;
}

int main()
{
    return client();
}