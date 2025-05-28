#ifndef SALE_PROCESS_H
#define SALE_PROCESS_H

#include <../structs/sale.h>

/**
 * @brief Gère l'entrée utilisateur pour initier une vente ou placer une enchère.
 * @param res Résultat du choix utilisateur (1 = init vente, 2 = enchérir).
 * @param sales_list Tableau des ventes.
 * @param cmpt_sale Pointeur vers le compteur de ventes.
 * @param vente Pointeur vers l'état de la vente.
 * @param udp_sock Socket UDP utilisé pour la communication.
 * @param peers Liste des pairs.
 * @param last_enchere Pointeur vers le temps de la dernière enchère.
 * @return 0 en cas de succès, -1 en cas d'échec.
 */
int inputUserSale(int res, sale *sales_list, int *cmpt_sale, int *vente, int udp_sock, PeerList *peers, time_t *last_enchere);

/**
 * @brief Initialise une nouvelle vente et la diffuse aux pairs.
 * @param udpsock Socket UDP.
 * @param list Liste des pairs.
 * @param sale_list Tableau des ventes.
 * @param cmpt_sale Pointeur vers le compteur de ventes.
 * @param prixb Prix de base de la vente.
 * @return 0 en cas de succès, -1 en cas d'échec.
 */
int sale_start(int udpsock, PeerList *list, sale *sale_list, int *cmpt_sale, int prixb);

/**
 * @brief Prépare et envoie une enchère sur une vente.
 * @param info Tableau contenant [prix, num_vente].
 * @param udp_sock Socket UDP.
 * @param list Liste des pairs.
 * @param sales_list Tableau des ventes.
 * @param cmpt_sale Nombre de ventes.
 * @return Pointeur vers le message créé, ou NULL en cas d'échec.
 */
message *encherir(uint32_t info[2], int udp_sock, PeerList *list, sale *sales_list, int cmpt_sale);

/**
 * @brief Affiche les ventes actives et demande à l'utilisateur un prix et un numéro de vente.
 * @param info Tableau où seront stockés le prix et le numéro de vente saisis.
 * @param sales_list Tableau des ventes.
 * @param cmpt_sale Nombre de ventes.
 */
void demander_prix_numvente(uint32_t info[2], sale *sales_list, int cmpt_sale);

/**
 * @brief Gère la finalisation d'une enchère (clôture de la vente).
 * @param msg_copy Message de la derniere enchère.
 * @param sales_list Tableau des ventes.
 * @param udp_sock Socket UDP.
 * @param cmpt_sale Nombre de ventes.
 * @param nombre_peer Nombre de pairs.
 * @param l Liste des pairs.
 */
void finalisation_enchere(message *msg_copy, sale *sales_list, int udp_sock, int cmpt_sale, int nombre_peer, PeerList *l);

/**
 * @brief Envoie une alerte de fin d'enchère si aucune enchère n'a été reçue depuis un certain temps.
 * @param msg_copy Message de la derniere enchère.
 * @param sales_list Tableau des ventes.
 * @param udp_sock Socket UDP.
 * @param last_enchere Pointeur vers le temps de la dernière enchère.
 * @param cmpt_sale Nombre de ventes.
 * @return 0 en cas de succès, -1 en cas d'échec.
 */
int alerte_fin_enchere(message *msg_copy, sale *sales_list, int udp_sock, time_t *last_enchere, int cmpt_sale);

/**
 * @brief Vérifie la validité d'une enchère reçue et gère la logique de consensus.
 * @param msg_copy Message contenant l'enchère.
 * @param sales_list Tableau des ventes.
 * @param last_enchere Pointeur vers le temps de la dernière enchère.
 * @param me_id Identifiant du pair local.
 * @param udp_sock Socket UDP.
 * @param cmpt_sale Nombre de ventes.
 * @param nombre_peer Nombre de pairs.
 * @param l Liste des pairs.
 * @return 0 en cas de succès, -1 en cas d'échec.
 */
int verifier_enchere(message *msg_copy, sale *sales_list, time_t *last_enchere, uint16_t me_id, int udp_sock, int cmpt_sale, int nombre_peer, PeerList *l);

#endif