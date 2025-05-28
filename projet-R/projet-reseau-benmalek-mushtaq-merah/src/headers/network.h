// network.h
#ifndef NETWORK_H
#define NETWORK_H
#include "./structs/message.h"
#include <stdint.h>
#include <netinet/in.h>

/**
 * @brief Crée un socket UDP IPv6, le configure pour le multicast et rejoint le groupe.
 *
 * @param mcast_addr Adresse IPv6 du groupe multicast (ex : "ff15::1").
 * @param port Port d'écoute.
 * @param iface Nom de l'interface réseau (ex : "eth0").
 * @return Descripteur du socket ou -1 en cas d'erreur.
 */
int create_udp_socket(char *mcast_addr, uint16_t port, char *iface);

/**
 * @brief Crée un socket TCP IPv6 en écoute sur le port donné.
 *
 * @param port Port d'écoute.
 * @return Descripteur du socket ou -1 en cas d'erreur.
 */
int create_tcp_listener(uint16_t port);

/**
 * @brief Accepte une connexion entrante sur un socket TCP en écoute.
 *
 * @param listen_sock Descripteur du socket d'écoute.
 * @return Descripteur du socket client ou -1 en cas d'erreur.
 */
int accept_tcp_connection(int listen_sock);

/**
 * @brief Établit une connexion TCP vers un pair à l'adresse IPv6 et au port spécifiés.
 *
 * @param ip Adresse IPv6 du pair (tableau de 16 octets).
 * @param port Port du pair.
 * @return Descripteur du socket ou -1 en cas d'erreur.
 */
int connect_to_peer(uint8_t ip[16], uint16_t port);
/**
 * @brief Envoie un message via un socket (TCP).
 *
 * @param sock Descripteur du socket.
 * @param len Taille du buffer.
 * @param buf Buffer à envoyer.
 * @return Nombre d'octets envoyés ou -1 en cas d'erreur.
 */
ssize_t send_message(int sock, int len, char *buf);

/**
 * @brief Reçoit un message sur un socket TCP avec timeout.
 *
 * @param sock Descripteur du socket.
 * @param buf Buffer de réception.
 * @param timeout_sec Timeout en secondes.
 * @return Nombre d'octets reçus, 0 si fermé, -1 en cas d'erreur.
 */
int receive_message_tcp(int sock, char *buf, int timeout_sec);

/**
 * @brief Envoie un message UDP à un groupe multicast.
 *
 * @param udp_sock Descripteur du socket UDP.
 * @param buffer Données à envoyer.
 * @param len Taille des données.
 * @param ip Adresse IPv6 du groupe multicast.
 * @param port Port du groupe.
 * @return Nombre d'octets envoyés ou -1 en cas d'erreur.
 */
ssize_t send_message_udp_multicast(int udp_sock, char *buffer, int len, char *ip, int port);

/**
 * @brief Envoie un message UDP à une adresse unicast.
 *
 * @param udp_sock Descripteur du socket UDP.
 * @param msg Pointeur vers la structure message à envoyer.
 * @param dest_addr Adresse de destination.
 * @return 0 si succès, -1 en cas d'erreur.
 */
int send_message_udp_unicast(int udp_sock, message *msg, struct sockaddr_in6 *dest_addr);
/**
 * @brief Affiche une adresse IPv6 sous forme lisible.
 *
 * @param ip Adresse IPv6 (16 octets).
 */
void print_ipv6_address(uint8_t ip[16]);
/**
 * @brief Ferme un socket s'il est ouvert.
 *
 * @param sock Descripteur du socket.
 */
void close_socket(int sock);

/**
 * @brief Reçoit un message UDP, extrait l'adresse source et convertit le buffer en structure message.
 *
 * @param sock Descripteur du socket UDP.
 * @param msg Pointeur vers un pointeur de message (rempli à la réception).
 * @param src_addr Adresse source (remplie à la réception).
 * @param addrlen Taille de l'adresse source.
 * @param timeout_sec Timeout en secondes.
 * @return 1 si succès, 0 si timeout, -1 en cas d'erreur.
 */
int receive_message_udp_and_extract_adr(int sock, message **msg, struct sockaddr_in6 *src_addr, socklen_t *addrlen, int timeout_sec);
#endif