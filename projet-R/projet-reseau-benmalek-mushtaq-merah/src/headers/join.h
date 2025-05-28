#ifndef JOIN_H
#define JOIN_H
#include "./structs/message.h"
#include "./structs/peerList.h"

#include <stdint.h>
#include <netinet/in.h>

int send_join_request(int udp_sock, message *join_msg);
int add_responding_peer(PeerList *peers, message *response);
int try_connect_and_handshake(PeerList *peers, Peer *peer);
PeerList *receive_peer_candidates(int udp_sock);
void handle_peer_list_sync(int tcp_sock, PeerList *peers);
int join_existing_network(PeerList *peers, int udp_sock);
void handle_code6_message(message *msg, PeerList *peers);
void handle_incoming_tcp(int tcp_sock, PeerList *peers);
void send_peer_list(int sock, PeerList *peers);
void add_peer_to_list(PeerList *peers, message *request);
void broadcast_to_peers(PeerList *peers, message *msg, uint16_t exclude_id);
void handle_new_peer_connection(int client_sock, message *request, PeerList *peers);
void initialize_1st_peer_info(PeerList *peers);
int set_own_ipv6_address(PeerList *peers);
void initialize_multicast_info(PeerList *peers);
int initialize_sockets(int *udp_sock, int *tcp_sock, int *udp_enchere);
#endif
