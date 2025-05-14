#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include "req.h"

entete *remplissage_entete_req()
{
    entete *e = malloc(sizeof(entete));
    memset(e, 0, sizeof(entete));
    srand(time(NULL));
    e->ID = htons(rand() % 65535);
    // e->type = (1 << 8); // que RD = 1
    e->type = htons(0x0100);  // RD=1, Opcode=0, etc.
    e->QDCOUNT = htons(1);
    return e;
}

requete *remplissage_requete(char *dom)
{
    entete *e = remplissage_entete_req();
    requete *r = malloc(sizeof(requete));
    memset(r, 0, sizeof(requete));
    r->e = *(e);
    char *copy = malloc(strlen(dom) + 1);
    if (copy == NULL)
    {
        perror("Erreur d'allocation mémoire pour copy");
        return NULL;
    }
    strcpy(copy, dom);
    char *token = strtok(copy, ".");
    char adresse_mess[ARG_MAX];
    int pos = 0;
    while (token != NULL)
    {
        int taille = strlen(token);
        adresse_mess[pos] = taille;
        pos++;
        for (int i = 0; token[i] != '\0'; i++)
        {
            adresse_mess[pos] = token[i];
            pos++;
        }
        token = strtok(NULL, ".");
    }
    adresse_mess[pos] = 0x00;
    pos++;
    r->Qname = malloc(pos);
    memcpy(r->Qname, adresse_mess, pos);
    r->Qtype = htons(1);
    r->Qclass = htons(1);
    return r;
}

int struct2buf(requete rq, char *buf)
{
    // & on compte en octets
    int index = 0;
    // ^ entete
    memcpy(buf, &(rq.e.ID), 2);
    index += 2;
    memcpy(buf + index, &(rq.e.type), 2);
    index += 2;
    memcpy(buf + index, &(rq.e.QDCOUNT), 2);
    index += 2;
    memcpy(buf + index, &(rq.e.ANCOUNT), 2);
    index += 2;
    memcpy(buf + index, &(rq.e.NSCOUNT), 2);
    index += 2;
    memcpy(buf + index, &(rq.e.ARCOUNT), 2);
    index += 2;

    // ^corps de la requete
    // Calculez la taille réelle de Qname (jusqu'au 0x00 final)
    int qname_len = 0;
    while (rq.Qname[qname_len] != 0x00)
    {
        qname_len += rq.Qname[qname_len] + 1;
    }
    qname_len++; // Inclure le 0x00 final

    memcpy(buf + index, rq.Qname, qname_len); // ✅ Copie tous les octets y compris les 0x00
    index += qname_len;
    memcpy(buf + index, &(rq.Qtype), 2); // copier 2 octets
    index += 2;
    memcpy(buf + index, &(rq.Qclass), 2); // copier 2 octets
    index += 2;

    return index;
}

requete *buf2struct(char *buf, int buf_size)
{
    if (buf == NULL || buf_size <= 0)
    {
        fprintf(stderr, "Erreur : tampon invalide ou taille incorrecte\n");
        return NULL;
    }

    requete *r = malloc(sizeof(requete));
    if (r == NULL)
    {
        perror("Erreur d'allocation mémoire pour la requête");
        return NULL;
    }

    int index = 0;
    // Reconstruction de l'entête
    memcpy(&r->e.ID, buf + index, 2);
    index += 2;

    memcpy(&r->e.type, buf + index, 2);
    index += 2;

    memcpy(&r->e.QDCOUNT, buf + index, 2);
    index += 2;

    memcpy(&r->e.ANCOUNT, buf + index, 2);
    index += 2;

    memcpy(&r->e.NSCOUNT, buf + index, 2);
    index += 2;

    memcpy(&r->e.ARCOUNT, buf + index, 2);
    index += 2;

    // Reconstruction de Qname
    int qname_start = index;
    while (buf[index] != 0x00 && index < buf_size)
    {
        index++;
    }
    if (index >= buf_size)
    {
        fprintf(stderr, "Erreur : Qname mal formé\n");
        free(r);
        return NULL;
    }
    int qname_length = index - qname_start + 1; // Inclure le caractère de fin 0x00
    r->Qname = malloc(qname_length);
    if (r->Qname == NULL)
    {
        perror("Erreur d'allocation mémoire pour Qname");
        free(r);
        return NULL;
    }
    memcpy(r->Qname, buf + qname_start, qname_length);
    index++;

    // Reconstruction de Qtype
    memcpy(&r->Qtype, buf + index, 2);
    index += 2;

    // Reconstruction de Qclass
    memcpy(&r->Qclass, buf + index, 2);
    index += 2;

    return r;
}
int close_req(requete *r)
{
    if (r != NULL)
    {
        if (r->Qname != NULL)
        {
            free(r->Qname);
        }
        free(r);
    }
    return 0;
}
