#ifndef REQ_H
#define REQ_H

#include <stdint.h>

// Définition des constantes
#define ARG_MAX 1500 // & MTU

// Définition des structures
typedef struct
{
    uint16_t ID;
    uint16_t type;
    uint16_t QDCOUNT;
    uint16_t ANCOUNT;
    uint16_t NSCOUNT;
    uint16_t ARCOUNT;
} entete;

typedef struct
{
    entete e;
    char *Qname;
    uint16_t Qtype;
    uint16_t Qclass;
} requete;

// Prototypes des fonctions
entete *remplissage_entete_req();
requete *remplissage_requete(char *dom);
int close_req(requete *r);
int struct2buf(requete rq, char *buf);
requete *buf2struct(char *buf, int buf_size);

#endif // REQ_H