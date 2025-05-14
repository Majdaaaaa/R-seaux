#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SIZE_MSG 1024
#define ADDR "lampe"
#define port "2628"
#define end "\r\n"

union adresse
{
    char adr4[INET_ADDRSTRLEN];
    char adr6[INET6_ADDRSTRLEN];
};

typedef struct
{
    char buf[2 * SIZE_MSG + 1];
    int cur;  // position de la ligne suivante dans le tampon
    int size; // taille courante du tampon
} buf_t;

int find_newline(buf_t *buf)
{
    char *pos = strstr(buf->buf, end); // end est défini comme "\r\n"
    if (pos != NULL)
    {
        int newline_index = pos - buf->buf + strlen(end);
        return newline_index;
    }
    else
    {
        return -1;
    }
}

int is_last_line(buf_t *buf)
{
    if (strncmp(buf->buf + buf->cur, "250", 3) == 0 || strncmp(buf->buf + buf->cur, "552", 3) == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void affiche_connexion(struct addrinfo *p)
{
    union adresse buf;
    if (p->ai_addrlen == sizeof(struct sockaddr_in))
    {
        inet_ntop(AF_INET, &(((struct sockaddr_in *)p->ai_addr)->sin_addr), buf.adr4, sizeof(buf.adr4));
        printf("adresse serveur : IP: %s port: %d\n", buf.adr4, ntohs(((struct sockaddr_in *)p->ai_addr)->sin_port));
    }
    else
    {
        inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)p->ai_addr)->sin6_addr), buf.adr6, sizeof(buf.adr6));
        printf("adresse serveur : IP: %s port: %d\n", buf.adr6, ntohs(((struct sockaddr_in6 *)p->ai_addr)->sin6_port));
    }
}

void handle_error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void send_message(int socket_fd, const char *message)
{
    int s = 0;
    while (s < strlen(message))
    {
        int r = send(socket_fd, message + s, strlen(message) - s, 0);
        if (r <= 0)
        {
            handle_error("send");
        }
        s += r;
    }
}

void receive_message(int socket_fd, char *buf, int size, const char *expected_start)
{
    int recu = 0;
    while (recu < 4)
    {
        recu += recv(socket_fd, buf + recu, size, 0);
        if (recu <= 0)
        {
            handle_error("recv");
        }
    }
    buf[recu] = '\0';

    if (strncmp(buf, expected_start, 4) != 0)
    {
        printf("La réponse du serveur ne commence pas par '%s'.\n", expected_start);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("La réponse du serveur commence par '%s'.\n", expected_start);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stdout, "usage : ./a.out <requete>\n");
        exit(EXIT_FAILURE);
    }
    char *mot[argc - 1];
    for (int i = 1; i < argc; i++)
    {
        mot[i - 1] = argv[i];
        printf("mot[%d] = %s\n", i - 1, mot[i - 1]);
    }
    int socket_fd;
    struct addrinfo hints, *r, *p;

    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_V4MAPPED | AI_ALL;
    hints.ai_protocol = 0;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ADDR, port, &hints, &r) != 0)
    {
        handle_error("getaddrinfo");
    }

    for (p = r; p != NULL; p = p->ai_next)
    {
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) >= 0)
        {
            if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == 0)
            {
                printf("Connexion réussie à : ");
                affiche_connexion(p);
                break;
            }
            printf("Connexion échouée à l'adresse : ");
            affiche_connexion(p);
            close(socket_fd);
        }
    }

    if (p == NULL)
    {
        exit(EXIT_FAILURE);
    }
    struct timeval tv;
    tv.tv_sec = 5; // 5 secondes de timeout
    tv.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);

    char buf[SIZE_MSG];
    receive_message(socket_fd, buf, SIZE_MSG, "220 ");

    char bufsend[SIZE_MSG] = {0};
    for (int i = 0; i < argc - 1; i++)
    {
        strncat(bufsend, mot[i], SIZE_MSG - strlen(bufsend) - 1);
        strncat(bufsend, " ", SIZE_MSG - strlen(bufsend) - 1);
    }
    strncat(bufsend, end, SIZE_MSG - strlen(bufsend) - 1);
    send_message(socket_fd, bufsend);

    buf_t buff;
    buff.cur = 0;
    buff.size = 0;
    memset(buff.buf, 0, sizeof(buff.buf));
    int new = 0;
    int fin = 0;
    while (1)
    {
        int r = recv(socket_fd, buff.buf + buff.size, sizeof(buff.buf) - buff.size, 0);

        if (r < 0)
        {
            handle_error("recv");
        }
        if (r == 0)
        {
            printf("Connexion fermée par le serveur.\n");
            break;
        }
        buff.size += r;
        buff.buf[buff.size] = '\0';
        while ((new = find_newline(&buff)) >= 0)
        {
            // Vérifie si la ligne actuelle est la dernière ligne (".\r\n")
            if (strncmp(buff.buf + buff.cur, ".\r\n", 3) == 0)
            {
                printf("\n");
                buff.cur += 3;
                continue;
            }
        
            // Vérifie si la ligne actuelle est une ligne de fin (par exemple, "250" ou "552")
            if (is_last_line(&buff) == 0 && strstr(buff.buf + buff.cur, "\r\n") != NULL)
            {
                printf("%.*s", new - buff.cur, buff.buf + buff.cur);
                fin = 1;
                break;
            }
        
            // Affiche la ligne actuelle
            printf("%.*s\n", new - buff.cur, buff.buf + buff.cur);
        
            // Déplace les données restantes dans le tampon pour traiter les prochaines lignes
            memmove(buff.buf, buff.buf + new, buff.size - new);
            buff.size -= new;
            buff.cur = 0;
            buff.buf[buff.size] = '\0';
        }
        
        if (fin == 1)
        {
            break;
        }
    }
    if (buff.size > buff.cur)
    {
        printf("%.*s\n", buff.size - buff.cur, buff.buf + buff.cur);
    }

    close(socket_fd);
    freeaddrinfo(r);
    return 0;
}