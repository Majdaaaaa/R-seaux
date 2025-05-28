#ifndef CONSENSUS_H
#define CONSENSUS_H

#include "peers.h"
#include "message.h"
#include "join.h"
#include "network.h"
#include "messages.h"
#include "peer.h"
#include <string.h>
#include <openssl/evp.h>
#include <message.h>
#include <messages.h>
#include <stdbool.h>

/**
 * @brief Vérifie si un pair a pas repondu au message de consensus.
 *
 * @param tab Tableau d'identifiants.
 * @param id Identifiant à vérifier.
 * @param size Taille du tableau.
 * @return true si le pair a pas repondud, false sinon.
 */
bool pas_encore_rep(int *tab, int id, ssize_t size);

/**
 * @brief  Savoir si le pair a 
 * 
 * @param m_superviseur Message de référence.
 * @param buffer Buffer contenant le message original.
 * @param recu Message reçu.
 * @param ont_rep Tableau des pairs ayant déjà répondu.
 * @param rep_cmp Pointeur sur le nombre de réponses reçues.
 * @return true si la réponse est valide et nouvelle, false sinon.
 */
bool update_reponse(message *m_superviseur, char *buffer, message *recu, Peer *ont_rep, int *rep_cmp);

/**
 * @brief Initialise une structure sockaddr_in6 à partir d'un Peer.
 *
 * @param p Pair dont on veut l'adresse.
 * @return Structure sockaddr_in6 initialisée.
 */
struct sockaddr_in6 initialise_addr(Peer p);

/**
 * @brief Relance un message à un pair et attend une réponse.
 *
 * @param udp_sock Socket UDP.
 * @param m Message à envoyer.
 * @param p Pair destinataire.
 * @param recu Message reçu en réponse.
 * @return 1 si réponse reçue, 0 si timeout, -1 en cas d'erreur.
 */
int relance(int udp_sock, message *m, Peer p, message *recu);

/**
 * @brief Gère la relance des pairs n'ayant pas encore répondu.
 *
 * @param udp_sock Socket UDP.
 * @param doit_rep Tableau des pairs à relancer.
 * @param pas_rep_cmp Nombre de pairs à relancer.
 * @param ont_rep Tableau des pairs ayant répondu.
 * @param rep_cmp Pointeur sur le nombre de réponses reçues.
 * @param a_envoyer Message à envoyer.
 * @param disparus Tableau des pairs considérés comme disparus.
 * @param dispa_cmp Pointeur sur le nombre de pairs disparus.
 */
void section_g(int udp_sock, Peer *doit_rep, int pas_rep_cmp, Peer *ont_rep, int *rep_cmp, message *a_envoyer, Peer *disparus, int *dispa_cmp);

/**
 * @brief Lance le consensus entre tous les pairs.
 *
 * @param udp_sock Socket UDP.
 * @param m_pair_Q Tableau de messages à envoyer.
 * @param nb_mess Nombre de messages à envoyer.
 * @param total_Peer Nombre total de pairs.
 * @param peers Liste des pairs.
 * @return 0 en cas de succès, -1 en cas d'erreur.
 */
int consensus(int udp_sock, message **m_pair_Q, int nb_mess, int total_Peer, PeerList *peers);

#endif