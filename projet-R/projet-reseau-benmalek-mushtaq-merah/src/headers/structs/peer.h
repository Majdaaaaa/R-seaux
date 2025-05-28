// peer.h
#ifndef PEER_H
#define PEER_H

#include <stdint.h>
#include <netinet/in.h>
#include <time.h>
#include <stdbool.h>

typedef struct
{
    uint16_t id;
    uint8_t ip[16]; // IPv6 address
    uint16_t port;
    char public_key[114]; // ED25519 public key
    time_t last_seen;
    int cpt; // Compteur de combien de vente a initier le pair
    uint32_t vente_supervisee;
} Peer;

#endif