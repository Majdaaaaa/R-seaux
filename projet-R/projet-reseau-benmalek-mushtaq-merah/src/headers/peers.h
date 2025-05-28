#ifndef PEERS_H
#define PEERS_H

#include <stdint.h>
#include <netinet/in.h>
#include <time.h>
#include "./structs/peer.h"
#include "./structs/peerList.h"
#include "./structs/message.h"

/**
 * @brief Affiche la liste des pairs présents dans la PeerList.
 * @param list Pointeur vers la liste des pairs.
 */
void print_peer_list(const PeerList *list);

/**
 * @brief Affiche les informations d'un pair.
 * @param peer Pointeur vers la structure Peer à afficher.
 */
void print_peer_info(Peer *peer);

/**
 * @brief Crée une nouvelle PeerList avec une capacité initiale.
 * @param initial_capacity Capacité initiale de la liste.
 * @return Pointeur vers la nouvelle PeerList, ou NULL en cas d'échec.
 */
PeerList *peer_list_create(size_t initial_capacity);

/**
 * @brief Libère la mémoire allouée pour une PeerList.
 * @param list Pointeur vers la PeerList à détruire.
 */
void peer_list_destroy(PeerList *list);

/**
 * @brief Ajoute un pair à la PeerList.
 * @param list Pointeur vers la PeerList.
 * @param peer Pointeur vers le Peer à ajouter.
 * @return 0 en cas de succès, -1 en cas d'échec.
 */
int peer_list_add(PeerList *list, Peer *peer);

/**
 * @brief Supprime un pair de la PeerList selon son identifiant.
 * @param list Pointeur vers la PeerList.
 * @param id Identifiant du pair à supprimer.
 * @return 0 en cas de succès, -1 si le pair n'est pas trouvé.
 */
int peer_list_remove(PeerList *list, uint16_t id);

/**
 * @brief Recherche un pair dans la PeerList selon son identifiant.
 * @param list Pointeur vers la PeerList.
 * @param id Identifiant du pair à rechercher.
 * @return Pointeur vers le Peer trouvé, ou NULL si absent.
 */
Peer *peer_list_find(PeerList *list, uint16_t id);

/**
 * @brief Charge une liste de pairs à partir d'un tableau de messages.
 * @param list Pointeur vers la PeerList à remplir.
 * @param msg Tableau de pointeurs vers des messages contenant les pairs.
 * @return 0 en cas de succès, -1 en cas d'échec.
 */
int peer_list_load(PeerList *list, message **msg);

#endif