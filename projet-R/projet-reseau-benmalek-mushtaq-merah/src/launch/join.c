#include "../headers/join.h"

#include "../headers/peers.h"
#include "../headers/structs/config.h"
#include "../headers/messages.h"
#include "../headers/structs/message.h"
#include "../headers/network.h"
#include "../headers/struct_mess.h"
#include "../headers/utils.h"
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <features.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>

/******************************************************************************************************
***************************FCTS AUXILIARS POUR join_existing_network()
*****************************************************************************************************/

int send_join_request(int udp_sock, message *join_msg)
{
    char buffer[MTU];
    int r = struct2buf(join_msg, buffer);
    if (send_message_udp_multicast(udp_sock, buffer, r, LIAISON_IP, LIAISON_PORT) < 0)
    {
        perror("Failed to send join request");
        return -1;
    }
    return 0;
}

int add_responding_peer(PeerList *peers, message *response)
{
    Peer peer = {
        .id = response->id,
        .port = response->port};
    memcpy(peer.ip, response->ip, 16);
    return peer_list_add(peers, &peer);
}

/**
 *Creer une connexion TCP avec un pair et lui envoie le CODE 5 puis recoit un CODE 50 OU CODE 51
 */

int try_connect_and_handshake(PeerList *peers, Peer *peer)
{
    int tcp_sock = connect_to_peer(peer->ip, peer->port);
    if (tcp_sock < 0)
    {
        fprintf(stderr, "Failed to connect to peer\n");
        return -1;
    }

    // Message code 5
    message *conn_req = rejoindre_enchere_mess4(
        peers->self_id,
        peers->peers[0].ip,
        peers->peers[0].port, "");

    char buf[MTU];
    int r = struct2buf(conn_req, buf);
    if (send_message(tcp_sock, r, buf) < 0)
    {
        perror("Failed to send connection request");
        free_message(conn_req);
        close_socket(tcp_sock);
        return -1;
    }

    char buffer[MTU];
    int n = receive_message_tcp(tcp_sock, buffer, JOIN_TIMEOUT);
    if (n <= 0)
    {
        perror("Failed to receive ID confirmation");
        close_socket(tcp_sock);
        free_message(conn_req);
        return -1;
    }

    message *confirm_msg = buf2struct(buffer, &n);

    if (confirm_msg->code == 51)
    { // ID_CHANGED_CODE
        peers->self_id = confirm_msg->id;
        peers->peers[0].id = confirm_msg->id;
        printf("Assigned new ID: %d\n", peers->self_id);
    }
    free_message(conn_req);
    free_message(confirm_msg);
    return tcp_sock; // Return open socket if successful
}
/**
 *Si un reseau existe deja, on est cense recevoir un message UDP UNICAST CODE 4 (ici extraire l'adresse n'est pas important)
 */

PeerList *receive_peer_candidates(int udp_sock)
{
    PeerList *candidates = peer_list_create(10);
    time_t start = time(NULL);

    while (time(NULL) - start < 1)
    {
        message *response = NULL;
        struct sockaddr_in6 sender_addr;
        socklen_t addr_len = sizeof(sender_addr);

        /*here le "extract adress" is not import bc we dont use it , mais
        jai pas envie de reecrire la methode au complet sans le extract sinn ca sera lourd*/
        if (receive_message_udp_and_extract_adr(udp_sock, &response, &sender_addr, &addr_len, 1) > 0 && response->code == 4)
        {
            printf("Received peer info from ID %d\n", response->id);
            add_responding_peer(candidates, response);
        }
        free_message(response);
    }

    return candidates;
}

/**
Gere la reception du CODE 7 (prend la liste des pairs envoye avec leurs infos et l'ajoute a sa liste perso) */
void handle_peer_list_sync(int tcp_sock, PeerList *peers)
{
    char buf[MTU];
    int n = receive_message_tcp(tcp_sock, buf, JOIN_TIMEOUT);
    if (n <= 0)
    {
        printf("%s[ERREUR]%s Connexion fermée par le pair (aucun CODE 7 reçu)\n", COLOR_RED, COLOR_RESET);
        close_socket(tcp_sock);
        return;
    }
    size_t count = 0;
    message **msg = buf2structs(buf, &count, n);

    if (count > 0 && msg[0]->code == 7)
    {
        if (peer_list_load(peers, msg) == 0)
        {
            memcpy(peers->enchere_ip, msg[0]->ip, 16);
            peers->enchere_port = msg[0]->port;
            printf("%s[TCP]%s Liste des pairs synchronisée avec succès (%s%zu%s pairs).\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, peers->count, COLOR_RESET);
            print_peer_list(peers);
        }
        else
        {
            printf("%s[ERREUR]%s Échec du chargement de la liste des pairs\n", COLOR_RED, COLOR_RESET);
        }
    }
    else
    {
        printf("%s[ERREUR]%s Échec de la réception de la liste des pairs (CODE 7)\n", COLOR_RED, COLOR_RESET);
    }
    free_messages(msg, count);
    close_socket(tcp_sock);
}

/***************************************************************************************************
**************************************************************************************************** */

/**
 *Retourne 0 si on est le premier pair a rejoindre
 *On suppose que le pair P est celui qui essaie de rejoindre le reseau
 */

int join_existing_network(PeerList *peers, int udp_sock)
{
    message *join_msg = rejoindre_enchere_mess1(); /*On creer un message (CODE = 3 )*/

    for (int attempt = 0; attempt < MAX_JOIN_ATTEMPTS; attempt++)
    {
        printf("Attempt %d to join network...\n", attempt + 1);

        /*P (on) envoie le message CODE 3 a l'adresse liaison pour avertir "je veux me connecter/ creer un reseau"*/
        if (send_join_request(udp_sock, join_msg) < 0)
            continue;

        PeerList *candidates = receive_peer_candidates(udp_sock);

        // On tente de se connecter a chaque pair ayant repondu
        for (size_t i = 0; i < candidates->count; i++)
        {
            int tcp_sock = try_connect_and_handshake(peers, &candidates->peers[i]);

            if (tcp_sock < 0)
                continue;

            /*P recoit le CODE 7 = voici la liste des autres pairs sur le reseau et leurs infos */
            handle_peer_list_sync(tcp_sock, peers);

            peer_list_destroy(candidates);
            return 0;
        }

        peer_list_destroy(candidates);
    }

    return -1; // Failed to join
}

void handle_code6_message(message *msg, PeerList *peers)
{
    // Check if we already know this peer
    Peer *existing = peer_list_find(peers, msg->id);
    if (existing)
    {
        // Update existing peer info
        memcpy(existing->ip, msg->ip, 16);
        existing->port = msg->port;
        // strncpy(existing->public_key, msg->cle, sizeof(existing->public_key));
    }
    else
    {
        // Add new peer to our list
        Peer new_peer = {
            .id = msg->id,
            .port = msg->port};
        memcpy(new_peer.ip, msg->ip, 16);
        // strncpy(new_peer.public_key, msg->cle, sizeof(new_peer.public_key));
        peer_list_add(peers, &new_peer);
    }

    printf("Updated peer list with new peer ID %d\n", msg->id);
}

void handle_incoming_tcp(int tcp_sock, PeerList *peers)
{
    int client_sock = accept_tcp_connection(tcp_sock);
    if (client_sock < 0)
    {
        fprintf(stderr, "%s[ERREUR]%s Connexion TCP refusée\n", COLOR_RED, COLOR_RESET);
        return;
    }
    // printf("%s[TCP]%s Connexion acceptée (socket : %d)\n", COLOR_CYAN, COLOR_RESET, client_sock);

    char buf[MTU];
    int n = receive_message_tcp(client_sock, buf, -1);
    if (n <= 0)
    {
        printf("%s[TCP]%s Connexion fermée ou aucun message reçu.\n", COLOR_YELLOW, COLOR_RESET);
        close_socket(client_sock);
        return;
    }

    message *request = buf2struct(buf, &n);

    switch (request->code)
    {
    case 5:
        printf("%s[TCP]%s Connexion d'un nouveau pair...\n", COLOR_CYAN, COLOR_RESET);
        handle_new_peer_connection(client_sock, request, peers);
        break;
    case 6:
        printf("%s[TCP]%s Nouveau pair annoncé : %u\n", COLOR_CYAN, COLOR_RESET, request->id);
        handle_code6_message(request, peers);
        break;
    default:
        fprintf(stderr, "%s[TCP]%s %sCode inconnu : %d%s\n", COLOR_CYAN, COLOR_RESET, COLOR_RED, request->code, COLOR_RESET);
    }
    free_message(request);
    // printf("%s[TCP]%s Connexion client fermée (socket : %d)\n", COLOR_CYAN, COLOR_RESET, client_sock);
    close_socket(client_sock);
}

/********************************************************************************************
***************************FCts quxiliares de handle_incoming_tcp****************************
**********************************************************************************************/

void handle_new_peer_connection(int client_sock, message *request, PeerList *peers)
{
    message *response = malloc(sizeof(message));
    if (peer_list_find(peers, request->id))
    {
        uint16_t new_id = rand() % 65535 + 1;
        while (peer_list_find(peers, new_id))
            new_id++;
        response = identifiant_non_valide(new_id);
        printf("%s[TCP]%s ID déjà utilisé, attribution : %s%u%s\n", COLOR_YELLOW, COLOR_RESET, COLOR_CYAN, new_id, COLOR_RESET);
        request->id = new_id;
    }
    else
    {
        response->code = 50;
        printf("%s[TCP]%s ID accepté\n", COLOR_GREEN, COLOR_RESET);
    }
    char buf[MTU];
    int r = struct2buf(response, buf);
    send_message(client_sock, r, buf);

    message *peer_info_msg = rejoindre_enchere_mess5(request->id, request->ip, request->port, "");
    broadcast_to_peers(peers, peer_info_msg, request->id);

    add_peer_to_list(peers, request);
    printf("%s[TCP]%s Pair ajouté (ID : %u)\n", COLOR_CYAN, COLOR_RESET, request->id);
    send_peer_list(client_sock, peers);

    free_message(response);
    close_socket(client_sock);
}

void broadcast_to_peers(PeerList *peers, message *msg, uint16_t exclude_id)
{
    for (size_t i = 0; i < peers->count; i++)
    {
        if (peers->peers[i].id != peers->self_id && peers->peers[i].id != exclude_id)
        {
            int peer_sock = connect_to_peer(peers->peers[i].ip, peers->peers[i].port);
            if (peer_sock >= 0)
            {
                char buf[MTU];
                int r = struct2buf(msg, buf);
                send_message(peer_sock, r, buf);
                close_socket(peer_sock);
            }
        }
    }
}

void add_peer_to_list(PeerList *peers, message *request)
{
    if (request->id == peers->self_id)
        return;
    Peer new_peer = {
        .id = request->id,
        .port = request->port};
    memcpy(new_peer.ip, request->ip, 16);
    // strncpy(new_peer.public_key, request->cle, sizeof(new_peer.public_key));
    peer_list_add(peers, &new_peer);
}

void send_peer_list(int sock, PeerList *peers)
{
    char buf[MTU];
    message **response = rejoindre_enchere_mess6(peers);
    int r = structs2buf(response, peers->count + 1, buf);
    if (r > 0 && send_message(sock, r, buf) > 0)
        printf("%s[TCP]%s Liste des pairs envoyée (%s%zu%s pairs)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, peers->count, COLOR_RESET);
    free_messages(response, peers->count + 1);
    sleep(1);
}

/******************************************************************************************************
***************************FCTS AUXILIARES POUR LE MAIN
*****************************************************************************************************/

void initialize_1st_peer_info(PeerList *peers)
{
    peers->self_id = rand() % 65535 + 1;
    peers->peers[0].id = peers->self_id;
    peers->peers[0].port = LIAISON_PORT;
    peers->count = 1;
    // memset(peers->peers[0].public_key, 0, CLE_SIZE);
}

/*La seule adresse ip  qui nous interesse ets celle de l'interface eth0 */
int set_own_ipv6_address(PeerList *peers)
{
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs failed");
        return -1;
    }

    int ip_found = 0;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET6)
            continue;

        if (strcmp(ifa->ifa_name, "eth0") == 0)
        {
            struct sockaddr_in6 *addr = (struct sockaddr_in6 *)ifa->ifa_addr;
            memcpy(peers->peers[0].ip, &addr->sin6_addr, 16);
            ip_found = 1;
            break;
        }
    }
    freeifaddrs(ifaddr);
    if (!ip_found)
    {
        fprintf(stderr, "Could not find IPv6 address for eth0\n");
        return -1;
    }
    return 0;
}

/*Intialiser les scokets udp1 pour multicast et tcp pour ecoute */
int initialize_sockets(int *udp_sock, int *tcp_sock, int *udp_enchere)
{
    *udp_sock = create_udp_socket(LIAISON_IP, LIAISON_PORT, "eth0");
    *udp_enchere = create_udp_socket(ENCHERE_IP, ENCHERE_PORT, "eth0");
    *tcp_sock = create_tcp_listener(LIAISON_PORT);
    return (*udp_sock >= 0 && *tcp_sock >= 0 && *udp_enchere >= 0) ? 0 : -1;
}

void initialize_multicast_info(PeerList *peers)
{
    struct in6_addr enchere_addr;
    if (inet_pton(AF_INET6, ENCHERE_IP, &enchere_addr) != 1)
    {
        fprintf(stderr, "Failed to parse ENCHERE_IP\n");
        exit(EXIT_FAILURE);
    }
    memcpy(peers->enchere_ip, &enchere_addr, 16);
    peers->enchere_port = ENCHERE_PORT;
}