#include "../headers/peers.h"
#include "../headers/network.h"
#include "../headers/struct_mess.h"
#include <stdlib.h>
#include "struct_mess.h"
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <stdbool.h>

void print_peer_info(Peer *peer)
{
    char ip_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &peer->ip, ip_str, sizeof(ip_str));

    printf("Peer Information:\n");
    printf("-----------------\n");
    printf("ID: %u\n", peer->id);
    printf("IP Address: %s\n", ip_str);
    printf("Port: %d\n", peer->port);
}

PeerList *peer_list_create(size_t initial_capacity)
{
    PeerList *list = malloc(sizeof(PeerList));
    if (!list)
        return NULL;

    list->peers = malloc(initial_capacity * sizeof(Peer));
    if (!list->peers)
    {
        free(list);
        return NULL;
    }

    list->capacity = initial_capacity;
    list->count = 0;
    list->self_id = 0;
    memset(list->enchere_ip, 0, 16);
    list->enchere_port = 0;

    return list;
}

void peer_list_destroy(PeerList *list)
{
    if (list)
    {
        free(list->peers);
        free(list);
    }
}

int peer_list_add(PeerList *list, Peer *peer)
{
    if (!list || !peer)
        return -1;

    // Check if peer already exists
    for (size_t i = 0; i < list->count; i++)
    {
        if (list->peers[i].id == peer->id)
        {
            if (i == 0)
            {
                return -1;
            }
            printf("Updating existing peer: ID %u\n", peer->id);
            // Update existing peer
            list->peers[i] = *peer;
            return 0;
        }
    }

    // Resize if needed
    if (list->count >= list->capacity)
    {
        size_t new_capacity = list->capacity * 2;
        Peer *new_peers = realloc(list->peers, new_capacity * sizeof(Peer));
        if (!new_peers)
            return -1;

        list->peers = new_peers;
        list->capacity = new_capacity;
    }

    // Add new peer
    list->peers[list->count] = *peer;
    list->count++;

    return 0;
}

int peer_list_remove(PeerList *list, uint16_t id)
{
    if (!list)
    {
        return -1;
    }

    for (size_t i = 0; i < list->count; i++)
    {
        if (list->peers[i].id == id)
        {
            // Move last element to this position
            if (i < list->count - 1)
            {
                list->peers[i] = list->peers[list->count - 1];
            }
            list->count--;
            return 0;
        }
    }
    return -1; // Not found
}

Peer *peer_list_find(PeerList *list, uint16_t id)
{
    if (!list)
        return NULL;

    for (size_t i = 0; i < list->count; i++)
    {
        if (list->peers[i].id == id)
        {
            return &list->peers[i];
        }
    }

    return NULL;
}
void print_peer_list(const PeerList *list)
{
    if (!list)
    {
        printf("PeerList is NULL\n");
        return;
    }

    printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                PEER LIST SUMMARY                        \n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");

    // Print self information
    char self_ip_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, list->peers[0].ip, self_ip_str, sizeof(self_ip_str));
    printf("║ Self ID: %-6" PRIu16 " %-15s:%-5" PRIu16 "          \n",
           list->self_id, self_ip_str, list->peers[0].port);

    char mcast_ip_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, list->enchere_ip, mcast_ip_str, sizeof(mcast_ip_str));
    printf("║ Multicast: %-39s:%-5" PRIu16 "\n", mcast_ip_str, list->enchere_port);

    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║ Connected Peers: %-30zu         \n", list->count - 1); // Subtract self

    if (list->count > 1)
    {
        printf("╠═══════════════════════════════════════════════════════════════╣\n");
        printf("║ %-4s %-36s %-5s       \n", "ID", "IPv6 Address", "    Port");
        printf("╠═══════════════════════════════════════════════════════════════╣\n");

        for (size_t i = 1; i < list->count; i++)
        {
            char ip_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, list->peers[i].ip, ip_str, sizeof(ip_str));
            printf("║ %-4" PRIu16 " %-36s %-5" PRIu16 "      \n",
                   list->peers[i].id, ip_str, list->peers[i].port);
        }
    }

    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
}

int peer_list_load(PeerList *peers, message **msg)
{
    if (!peers || !msg || msg[0]->code != 7)
        return -1;

    // Sauvegarde infos self
    uint16_t self_id = peers->self_id;
    uint8_t self_ip[16];
    uint16_t self_port = 0;
    if (peers->count > 0)
    {
        memcpy(self_ip, peers->peers[0].ip, 16);
        self_port = peers->peers[0].port;
    }

    if (!peers)
        peers->count = 0;

    // Charger tous les pairs reçus (msg[1] à msg[nb])
    for (int i = 1; i <= msg[0]->nb; i++)
    {
        Peer p;
        p.id = msg[i]->id;
        memcpy(p.ip, msg[i]->ip, 16);
        p.port = msg[i]->port;
        strncpy(p.public_key, msg[i]->cle, CLE_SIZE - 1);
        peer_list_add(peers, &p);
    }

    // Vérifier si self est dans la liste, sinon l'ajouter
    if (!peer_list_find(peers, self_id))
    {
        Peer self;
        self.id = self_id;
        memcpy(self.ip, self_ip, 16);
        self.port = self_port;
        memset(self.public_key, 0, CLE_SIZE);
        peer_list_add(peers, &self);
    }

    // Mettre à jour enchere_ip/port
    memcpy(peers->enchere_ip, msg[0]->ip, 16);
    peers->enchere_port = msg[0]->port;

    return 0;
}