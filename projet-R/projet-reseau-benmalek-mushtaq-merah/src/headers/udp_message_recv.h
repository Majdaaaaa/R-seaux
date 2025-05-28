#ifndef UDP_MESSAGE_RECV_H
#define UDP_MESSAGE_RECV_H

#include "../headers/peers.h"
#include "structs/sale.h"

/**
 * @brief Traite la réception d'un message UDP et effectue l'action appropriée selon le code du message.
 * @param udp_sock Socket UDP.
 * @param peers Liste des pairs.
 * @param sales_list Tableau des ventes.
 * @param cmpt_sale Pointeur vers le compteur de ventes.
 * @param vente Pointeur vers l'état de la vente.
 * @param last_enchere Pointeur vers le temps de la dernière enchère.
 * @return Pointeur vers le message reçu, ou NULL si aucun message.
 */
message *handle_udp_message(int udp_sock, PeerList *peers, sale *sales_list, int *cmpt_sale, int *vente, time_t *last_enchere);

/**
 * @brief Gère une demande de connexion d'un nouveau pair.
 * @param udp_sock Socket UDP.
 * @param peers Liste des pairs.
 * @param sender_addr Adresse du pair demandeur.
 */
void handle_join_request(int udp_sock, PeerList *peers, struct sockaddr_in6 *sender_addr);

/**
 * @brief Gère l'initialisation d'une nouvelle vente à partir d'un message reçu.
 * @param sales_list Tableau des ventes.
 * @param cmpt_sale Pointeur vers le compteur de ventes.
 * @param msg Message reçu contenant les infos de la vente.
 */
void handle_sale_init(sale *sales_list, int *cmpt_sale, message *msg);

/**
 * @brief Gère la réception d'une offre d'enchère par le pair.
 * @param peers Liste des pairs.
 * @param sales_list Tableau des ventes.
 * @param vente Pointeur vers l'état de la vente.
 * @param last_enchere Pointeur vers le temps de la dernière enchère.
 * @param cmpt_sale Pointeur vers le compteur de ventes.
 * @param msg Message reçu contenant l'offre.
 * @return 1 si l'offre est traitée, 0 sinon.
 */
int handle_bid(PeerList *peers, sale *sales_list, int *vente, time_t *last_enchere, int *cmpt_sale, message *msg);

/**
 * @brief Gère la réception d'une nouvelle enchère par le superviseur et met à jour la vente.
 * @param peers Liste des pairs.
 * @param sales_list Tableau des ventes.
 * @param cmpt_sale Pointeur vers le compteur de ventes.
 * @param msg Message reçu contenant l'enchère.
 * @param udp_sock Socket UDP.
 * @param buffer Buffer contenant le message brut.
 * @param sender_addr Adresse de l'expéditeur.
 */
void handle_new_bid(PeerList *peers, sale *sales_list, int *cmpt_sale, message *msg, int udp_sock, char *buffer, struct sockaddr_in6 sender_addr);

/**
 * @brief Gère l'annonce de clôture d'une vente.
 * @param msg Message reçu contenant l'annonce.
 */
void handle_closure_announce(message *msg);

/**
 * @brief Gère la clôture effective d'une vente.
 * @param peers Liste des pairs.
 * @param msg Message reçu contenant la clôture.
 * @param buffer Buffer contenant le message brut.
 * @param sender_addr Adresse de l'expéditeur.
 * @param udp_sock Socket UDP.
 * @param sales_list Tableau des ventes.
 * @param cmpt Nombre de ventes.
 */
void handle_closure(PeerList *peers, message *msg, char *buffer, struct sockaddr_in6 sender_addr, int udp_sock, sale *sales_list, int cmpt);

/**
 * @brief Gère la liaison UDP lors de l'arrivée d'un nouveau pair.
 * @param udp_sock Socket UDP.
 * @param peers Liste des pairs.
 * @return 0 en cas de succès, -1 en cas d'échec ou d'absence de message.
 */
int handle_udp_liaison(int udp_sock, PeerList *peers);

/**
 * @brief Gère le départ d'un pair du réseau.
 * @param peers Liste des pairs.
 * @param msg Message reçu concernant le départ.
 * @param buffer Buffer contenant le message brut.
 * @param sender_addr Adresse de l'expéditeur.
 * @param udp_sock Socket UDP.
 * @param sales_list Tableau des ventes.
 * @param cmpt Nombre de ventes.
 */
void handle_peer_departure(PeerList *peers, message *msg, char *buffer, struct sockaddr_in6 sender_addr, int udp_sock, sale *sales_list, int cmpt);

/**
 * @brief Gère la disparition de plusieurs pairs du réseau.
 * @param peers Liste des pairs.
 * @param msg Tableau de pointeurs vers les messages reçus.
 * @param buffer Buffer contenant le message brut.
 * @param sender_addr Adresse de l'expéditeur.
 * @param udp_sock Socket UDP.
 */
void handle_disparition(PeerList *peers, message **msg, char *buffer, struct sockaddr_in6 sender_addr, int udp_sock);

/**
 * @brief Gère la réception d'une mauvaise enchère (prix trop bas).
 * @param msg Message reçu contenant l'enchère refusée.
 */
void handle_mauvaise_enchere(message *msg);

#endif