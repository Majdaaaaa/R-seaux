#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "../headers/structs/peerList.h"
#include "../headers/peers.h"
#include "../headers/messages.h"

// & processus de consensus
// Message envoyé par tous les pairs au superviseur
message *processus_consensus_mess1(uint16_t id, char *mess, char *sig)
{
    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;
    if (strlen(mess) > MESS_SIZE || strlen(sig) > SIG_SIZE)
    {
        fprintf(stderr, "Taille du message ou de la clé trop grande.\n");
        return NULL;
    }
    initialize_message(m);
    m->code = 1;
    m->id = id;
    m->lmess = strlen(mess);
    m->lsig = strlen(sig);
    strncpy(m->mess, mess, MESS_SIZE - 1);
    strncpy(m->sig, sig, SIG_SIZE);

    return m;
}

// Message envoyé par le superviseur
message **processus_consensus_mess4(uint16_t nb, char *mess, char *sig, u_int16_t id)
{

    if (nb == 0)
        return NULL;

    message **msgs = malloc(sizeof(message *) * (nb + 1));
    if (!msgs)
        return NULL;

    if (strlen(mess) > MESS_SIZE || strlen(sig) > SIG_SIZE)
    {
        fprintf(stderr, "Taille du message ou de la signature trop grande.\n");
        return NULL;
    }

    // Message principal (superviseur)
    msgs[0] = malloc(sizeof(message));
    if (!msgs[0])
    {
        free(msgs);
        return NULL;
    }
    initialize_message(msgs[0]);
    msgs[0]->code = 2;
    msgs[0]->id = id; // ID du superviseur
    msgs[0]->nb = nb;
    msgs[0]->lmess = strlen(mess);
    msgs[0]->lsig = strlen(sig);
    strncpy(msgs[0]->mess, mess, MESS_SIZE - 1);
    strncpy(msgs[0]->sig, sig, SIG_SIZE - 1);

    if (nb <= 3)
    {
        for (int i = 1; i <= nb; i++)
        {
            msgs[i] = malloc(sizeof(message));
            initialize_message(msgs[i]);
            msgs[i] = signature_individuelle(i, "signature");
            if (!msgs[i])
            {
                free_messages(msgs, i);
                return NULL;
            }
        }
    }
    else
    {
        // TODO : à gérer plus tard > de 4 pairs
        msgs[0]->code = 20;
    }

    return msgs;
}

// Remplir une structure pour signature individuel
message *signature_individuelle(uint16_t id, char *sig)
{
    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;

    if (strlen(sig) > SIG_SIZE)
    {
        fprintf(stderr, "Taille de la signature trop grande.\n");
        return NULL;
    }
    initialize_message(m);
    m->id = id;
    m->lsig = strlen(sig);
    strncpy(m->sig, sig, m->lsig);

    return m;
}

// & rejoindre une enchere
message *rejoindre_enchere_mess1(void)
{
    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;

    initialize_message(m);
    m->code = 3;
    return m;
}

message *rejoindre_enchere_mess3(uint16_t id, uint8_t ip[16], uint16_t port)
{
    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;

    initialize_message(m);
    m->code = 4;
    m->id = id;
    memcpy(m->ip, ip, 16);
    m->port = port;

    return m;
}

message *identifiant_valide(void)
{
    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;

    initialize_message(m);
    m->code = 50;
    return m;
}

message *identifiant_non_valide(uint16_t id)
{
    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;

    initialize_message(m);
    m->code = 51;
    m->id = id;
    return m;
}

message *suffixe_info(uint16_t id, uint8_t ip[16], uint16_t port, char *cle)
{
    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;

    if (strlen(cle) > CLE_SIZE)
    {
        fprintf(stderr, "Taille de la clé trop grande.\n");
        return NULL;
    }

    initialize_message(m);
    m->id = id;
    memcpy(m->ip, ip, sizeof(m->ip));
    m->port = port;
    strncpy(m->cle, cle, CLE_SIZE - 1);
    return m;
}

message *rejoindre_enchere_mess4(uint16_t id, uint8_t ip[16], uint16_t port, char *cle)
{
    message *m = suffixe_info(id, ip, port, cle);
    if (!m)
        return NULL;

    m->code = 5;
    return m;
}

message *rejoindre_enchere_mess5(uint16_t id, uint8_t ip[16], uint16_t port, char *cle)
{
    if (strlen(cle) > CLE_SIZE)
    {
        fprintf(stderr, "Taille de la clé trop grande.\n");
        return NULL;
    }
    message *m = suffixe_info(id, ip, port, cle);
    if (!m)
        return NULL;

    m->code = 6;
    return m;
}

message **rejoindre_enchere_mess6(PeerList *peers)
{
    if (!peers || peers->count == 0)
        return NULL;

    // On alloue peers->count + 1 pour le message principal + chaque pair
    message **msgs = malloc(sizeof(message *) * (peers->count + 1));
    if (!msgs)
        return NULL;

    msgs[0] = malloc(sizeof(message));
    if (!msgs[0])
    {
        free(msgs);
        return NULL;
    }
    msgs[0]->code = 7;
    msgs[0]->id = peers->self_id;
    memcpy(msgs[0]->ip, peers->enchere_ip, sizeof(msgs[0]->ip));
    msgs[0]->port = peers->enchere_port;
    msgs[0]->nb = peers->count;

    for (size_t i = 0; i < peers->count; i++)
    {
        msgs[i + 1] = suffixe_info(
            peers->peers[i].id,
            peers->peers[i].ip,
            peers->peers[i].port,
            peers->peers[i].public_key);
        if (!msgs[i + 1])
        {
            free_messages(msgs, i + 1);
            return NULL;
        }
    }

    return msgs;
}

// & déroulement d'une vente
message *initier_vente(uint16_t id, uint32_t numv, uint32_t prix)
{
    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;

    initialize_message(m);
    m->code = 8;
    m->id = id;
    m->numv = numv;
    m->prix = prix;
    return m;
}

message *encherir_vente(uint16_t id, uint32_t numv, uint32_t prix)
{
    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;

    initialize_message(m);
    m->code = 9;
    m->id = id;
    m->numv = numv;
    m->prix = prix;
    return m;
}

void encherir_superviseur(message *m)
{
    if (!m)
        return;
    m->code = 10;
}

// & finalisation de la vente

message *finalisation_mess1(uint16_t id, uint32_t numv, uint32_t prix)
{
    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;

    initialize_message(m);
    m->code = 11;
    m->id = id;
    m->numv = numv;
    m->prix = prix;
    return m;
}

message *finalisation_mess2(uint16_t id, uint32_t numv, uint32_t prix)
{
    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;

    initialize_message(m);
    m->code = 12;
    m->id = id;
    m->numv = numv;
    m->prix = prix;
    return m;
}

// & quitter le système

message *quitter_enchere(uint16_t id)
{
    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;

    initialize_message(m);
    m->code = 13;
    m->id = id;
    return m;
}

// & messages non valide

message *rejet_enchere(uint8_t code, uint16_t id, uint32_t numv, uint32_t prix)
{
    if (code != 14 && code != 15)
    {
        fprintf(stderr, "Code de rejet invalide: %u\n", code);
        return NULL;
    }

    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;

    initialize_message(m);
    m->code = code;
    m->id = id;
    m->numv = numv;
    m->prix = prix;
    return m;
}

// & gestion pairs qui repondent plus

message *disparition_superviseur(uint16_t id, uint32_t numv)
{
    message *m = malloc(sizeof(message));
    if (!m)
        return NULL;

    initialize_message(m);
    m->code = 16;
    m->id = id;
    m->numv = numv;
    return m;
}

message **disparition_pair(uint16_t id, uint16_t nb, Peer *disparus, int *cmp)
{
    message **m = malloc(sizeof(message) * (*cmp + 1));
    if (!m)
        return NULL;
    m[0] = malloc(sizeof(message));
    if (m[0] == NULL)
    {
        perror("malloc disparition_pair");
        return NULL;
    }
    initialize_message(m[0]);
    m[0]->code = 18;
    m[0]->id = id;
    m[0]->nb = nb;

    for (int i = 1; i <= *cmp; i++)
    {
        m[i] = malloc(sizeof(message));
        initialize_message(m[i]);
        m[i]->id = disparus[i - 1].id;
    }
    return m;
}