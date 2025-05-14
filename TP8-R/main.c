#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include "req.h"

#define ADDRDNS "192.168.70.200" // adresse dns de lulu
#define PORTDNS 53
#define ARG_MAX 1500 // & MTU

// ~ étape de création d'un client UDP :
// ~ crée la socket UDP
// ~ envoie et reception

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "usage : ./a.out <domain>\n");
        return 1;
    }

    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return 1;

    struct sockaddr_in adr;
    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    inet_pton(AF_INET, ADDRDNS, &adr.sin_addr);
    adr.sin_port = htons(PORTDNS);
    socklen_t len = sizeof(adr);

    requete *rq = remplissage_requete(argv[1]);
    if (rq == NULL)
        return 1;
    char *buf = malloc(ARG_MAX * sizeof(char));
    int size = struct2buf(*rq, buf);
    if (size <= 0)
    {
        perror("struct2buf");
        return 1;
    }
    char *tmp = realloc(buf, size * sizeof(char));
    if (tmp == NULL)
    {
        perror("realloc");
        return 1;
    }

    buf = tmp;

    int s = sendto(sock, buf, size, 0, (struct sockaddr *)&adr, len);
    if (s < 0)
    {
        perror("send");
        return 1;
    }
    printf("Requête envoyée (%d octets)\n", s);
    // Après sendto()
    printf("Requête hexadécimale : ");
    for (int i = 0; i < size; i++)
        printf("%02x ", (unsigned char)buf[i]);
    printf("\n");

    char recvbuf[ARG_MAX];
    int r = recvfrom(sock, recvbuf, ARG_MAX, 0, NULL, NULL);
    if (r < 0)
    {
        perror("recvfrom");
        return 1;
    }
    recvbuf[r] = '\0';
    r++;
    printf("taille recu : [%d]\n", r);
    printf("recu : [%s] \n", recvbuf);

    // ? je récupère l'entete et je regarde j'ai recu combien de réponse,
    // ? je saute tout les champs de la question et je récupère dans un buffer la réponse
    // requete *rqq = buf2struct(recvbuf, size);
    uint16_t anscount;
    memcpy(&anscount, recvbuf + 6, 2); // Octets 6-7 de l'en-tête
    anscount = ntohs(anscount);

    printf("nb de réponse : %d\n", anscount);

    int index = 12;
    // avancer dans QNAME
    while (recvbuf[index] != 0)
        index += recvbuf[index] + 1;
    index++; // pour le 0 final

    index += 4; // QTYPE (4 octets) + QCLASS (4 octets)

    if (anscount == 0)
    {
        printf("Pas de réponses.\n");
        unsigned char rcode = recvbuf[3] & 0x0F;
        if (rcode != 0)
        {
            printf("Erreur DNS, RCODE = %d\n", rcode);
            if (rcode == 3)
                printf("→ Nom de domaine inexistant (RCODE = 3)\n");
            else if (rcode == 1)
                printf("→ Format error\n");
            else if (rcode == 2)
                printf("→ Server failure\n");
            else if (rcode == 4)
                printf("→ Not implemented\n");
            else if (rcode == 5)
                printf("→ Refused\n");
            return 1;
        }
        return 0;
    }

    for (int i = 0; i < anscount; i++)
    {
        // NAME : lecture du pointeur de nom (compression)
        uint16_t name_ptr;
        memcpy(&name_ptr, recvbuf + index, 2);
        name_ptr = ntohs(name_ptr);
        printf("NAME (pointeur compressé) : 0x%x\n", name_ptr);
        index += 2;

        // TYPE
        uint16_t type;
        memcpy(&type, recvbuf + index, 2);
        type = ntohs(type);
        printf("TYPE : %d\n", type);
        index += 2;

        // CLASS
        uint16_t clas;
        memcpy(&clas, recvbuf + index, 2);
        clas = ntohs(clas);
        printf("CLASS : %d\n", clas);
        index += 2;

        // TTL (4 octets)
        uint32_t ttl;
        memcpy(&ttl, recvbuf + index, 4);
        ttl = ntohl(ttl);
        printf("TTL : %u\n", ttl);
        index += 4;

        // RDLENGTH
        uint16_t rdlength;
        memcpy(&rdlength, recvbuf + index, 2);
        rdlength = ntohs(rdlength);
        printf("RDLENGTH : %d\n", rdlength);
        index += 2;

        // RDATA
        if (type == 1 && rdlength == 4)
        { // IPv4
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, recvbuf + index, ip_str, sizeof(ip_str));
            printf("RDATA (IPv4) : %s\n", ip_str);
        }
        else if (type == 28 && rdlength == 16)
        { // IPv6
            char ip6_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, recvbuf + index, ip6_str, sizeof(ip6_str));
            printf("RDATA (IPv6) : %s\n", ip6_str);
        }
        else if (type == 5)
        { // CNAME
            printf("CNAME détecté. Nom canonique : ");
            int rdata_pos = 0;
            while (rdata_pos < rdlength)
            {
                uint8_t label_len = (uint8_t)recvbuf[index + rdata_pos];
                if (label_len == 0)
                    break;
                if ((label_len & 0xC0) == 0xC0)
                { // Pointeur de compression
                    uint16_t ptr = ntohs(*(uint16_t *)(recvbuf + index + rdata_pos)) & 0x3FFF;
                    // Décoder le nom à partir du pointeur (non implémenté ici)
                    printf("(compression non gérée)");
                    rdata_pos += 2;
                    break;
                }
                else
                {
                    for (int j = 1; j <= label_len; j++)
                    {
                        printf("%c", recvbuf[index + rdata_pos + j]);
                    }
                    rdata_pos += label_len + 1;
                    if (rdata_pos < rdlength)
                        printf(".");
                }
            }
            printf("\n");
        }
        else
        {
            printf("RDATA brute : ");
            for (int j = 0; j < rdlength; j++)
                printf("%.2x ", (unsigned char)recvbuf[index + j]);
            printf("\n");
        }

        index += rdlength; // avancer au prochain enregistrement
        printf("-------------------------------------------------\n");
    }

    close(sock);
    free(buf);
    return 0;
}