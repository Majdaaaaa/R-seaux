#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>    // Pour close(), read(), write()
#include <arpa/inet.h> // Pour htonl(), htons(), ntohl(), ntohs()
#include <sys/types.h>

#define PORT 9999
#define SIZE 1024
int serveur()
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }
    printf("socket crée\n");

    int yes = 1;
    int r = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (r < 0)
        fprintf(stderr, "échec de setsockopt() : (%d)\n", errno);

    struct sockaddr_in adr;
    adr.sin_family = AF_INET;
    adr.sin_port = htons(PORT);
    adr.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((bind(sock, (struct sockaddr *)&adr, sizeof(adr))) < 0)
    {
        perror("bind");
        close(sock);
        return 1;
    }
    printf("bind fait\n");

    if ((listen(sock, 0)) < 0)
    {
        perror("listen");
        close(sock);
        return 1;
    }

    printf("listen fait\n");

    int sockclient = sockclient = accept(sock, NULL, NULL);
    if (sockclient < 0)
    {
        perror("accept");
        close(sock);
        return 1;
    }
    printf("client connecté\n");

    char bufsend[SIZE];
    char bufrecv[SIZE];
    int bytes_recv = 0;
    int bytes_send = 0;
    sprintf(bufsend, "DEBUT\n");
    printf("Message à envoyé : %s\n", bufsend);

    // Je considère ici que tout est envoyé d'un coup
    // & faut envoyer et recevoir sur sockclient
    bytes_send = send(sockclient, bufsend, strlen(bufsend), 0);
    if (bytes_send < 0)
    {
        perror("send");
        close(sock);
        close(sockclient);
        return 1;
    }

    printf("Message envoyé\n");

    // Boucle de reception
    // 1er recv que la taille du fichier puis créer un buffer de la taille du fichier avec malloc (dynamique)
    while (1)
    {
        int r = recv(sockclient, bufrecv, SIZE - bytes_recv - 1, 0);
        if (r < 0)
        {
            perror("recv");
            close(sock);
            close(sockclient);
            return 1;
        }
        else if (r == 0)
        {
            perror("le client s'est déconnecté");
            close(sock);
            close(sockclient);
            return 1;
        }
        bytes_recv += r;
        bufrecv[bytes_recv] = 0;
        if (strstr(bufrecv, "\n") != NULL)
        {
            break;
        }
    }
    int taille; // Buffer pour stocker les caractères avant '\n'
    if (sscanf(bufrecv, "%d", &taille) == 1)
    {
        printf("Taille : %d\n", taille);
        printf("fichier : %s\n", strchr(bufrecv, '\n') + 1); // Afficher le contenu après '\n'
    }
    else
    {
        printf("Caractère '\\n' non trouvé dans le buffer.\n");
    }

    char *fic = malloc(taille * sizeof(char));
    sprintf(fic, strstr(bufrecv, "\n"));
    bytes_recv = strlen(fic) - 1;
    while (bytes_recv < taille)
    {
        int r = recv(sockclient, fic + bytes_recv, SIZE, 0);
        if (r < 0)
        {
            perror("recv");
            close(sock);
            close(sockclient);
            return 1;
        }
        else if (r == 0)
        {
            perror("le client s'est déconnecté");
            close(sock);
            close(sockclient);
            return 1;
        }
        bytes_recv += r;

        fic[bytes_recv] = 0;
    }

    int fd = open("data.txt", O_CREAT | O_WRONLY, 0777); // Ajouter O_WRONLY et corriger les permissions
    if (fd < 0)
    {
        perror("open");
        free(fic); // Libérer la mémoire allouée
        close(sock);
        close(sockclient);
        return 1;
    }
    printf("fichier crée\n");

    int total_written = 0;
    while (total_written < taille)
    {
        int w = write(fd, fic + total_written, taille - total_written);
        if (w < 0)
        {
            perror("write");
            free(fic); // Libérer la mémoire allouée
            close(sock);
            close(sockclient);
            close(fd);
            return 1;
        }
        total_written += w;
    }

    printf("écriture dans le fichier fini\n");

    free(fic);
    close(fd);
    close(sock);
    close(sockclient);
    return 0;
}

int main()
{
    return serveur();
}