
#ifndef PEERLIST_H
#define PEERLIST_H

#include <stdint.h>
#include <netinet/in.h>
#include <time.h>
#include "peer.h"

typedef struct {
    Peer *peers;
    size_t capacity;
    size_t count;
    uint16_t self_id;
    uint8_t enchere_ip[16];
    uint16_t enchere_port;
} PeerList;


#endif