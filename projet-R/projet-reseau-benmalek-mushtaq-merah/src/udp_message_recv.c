#include "headers/network.h"
#include "headers/peers.h"
#include "headers/udp_message_recv.h"
#include "headers/struct_mess.h"
#include "headers/structs/sale.h"
#include "headers/sales.h"
#include "headers/structs/config.h"
#include "headers/messages.h"
#include "headers/structs/message.h"
#include "headers/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>
#include "headers/sales.h"
#include "headers/structs/sale.h"
#include "headers/utils.h"

static uint32_t last_refused_numv = 0;
static uint16_t last_refused_id = 0;

message *handle_udp_message(int udp_sock, PeerList *peers, sale *sales_list, int *cmpt_sale, int *vente, time_t *last_enchere)
{
    message *msg = malloc(sizeof(message));
    initialize_message(msg);
    struct sockaddr_in6 sender_addr;
    socklen_t addr_len = sizeof(sender_addr);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(udp_sock, &fds);

    struct timeval tv = {5, 0};

    int ready = select(udp_sock + 1, &fds, NULL, NULL, &tv);
    if (ready <= 0)
    {
        return NULL;
    }

    char buffer[MTU];
    int n = recvfrom(udp_sock, buffer, MTU, 0, (struct sockaddr *)&sender_addr, &addr_len);
    if (n < 0)
    {
        perror("recvfrom failed");
        return NULL;
    }
    int r = 0;
    msg = buf2struct(buffer, &r);

    switch (msg->code)
    {
    case 2:
        printf("%s[CONSENSUS]%s Message de consensus reçu (code 2).\n", COLOR_BLUE, COLOR_RESET);
        break;
    case 4:
        printf("%s[INFO]%s Informations reçues du pair %s%u%s.\n", COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, msg->id, COLOR_RESET);
        break;
    case 8:
        handle_sale_init(sales_list, cmpt_sale, msg);
        break;
    case 9:
        if (handle_bid(peers, sales_list, vente, last_enchere, cmpt_sale, msg))
            return msg;
        break;
    case 10:
        handle_new_bid(peers, sales_list, cmpt_sale, msg, udp_sock, buffer, sender_addr);
        break;
    case 11:
        handle_closure_announce(msg);
        break;
    case 12:
        handle_closure(peers, msg, buffer, sender_addr, udp_sock, sales_list, *cmpt_sale);
        break;
    case 13:
        handle_peer_departure(peers, msg, buffer, sender_addr, udp_sock, sales_list, *cmpt_sale);
        break;
    case 16:
        printf("%s[DISPARITION]%s Le superviseur disparu !!\n", PURPLE, COLOR_RESET);
        break;
    case 18:
        size_t count = 0;
        message **m = buf2structs(buffer, &count, n);
        printf("%s[DISPARITION]%s Des pairs ont disparu !!\n", PURPLE, COLOR_RESET);
        handle_disparition(peers, m, buffer, sender_addr, udp_sock);
        break;
    case 15:
        handle_mauvaise_enchere(msg);
        break;
    case 14:
        printf("%s[REFUS ENCHÈRE]%s L'enchère a été %sREFUSÉE%s car concurrente avec une autre enchère !\n", COLOR_RED, COLOR_RESET, COLOR_RED, COLOR_RESET);
        printf("%s→ Vente #%s%u%s | Offre : %s%u%s | Pair demandeur : %s%u%s\n",
               COLOR_CYAN, COLOR_YELLOW, msg->numv, COLOR_CYAN,
               COLOR_LIGHTGREEN, msg->prix, COLOR_CYAN,
               COLOR_YELLOW, msg->id, COLOR_RESET);
        printf("%s----------------------------------------%s\n", COLOR_RED, COLOR_RESET);
        break;
    default:
        printf("%s[INFO]%s Code message inconnu : %d\n", COLOR_RED, COLOR_RESET, msg->code);
        break;
    }
    return msg;
}

void handle_mauvaise_enchere(message *msg)
{
    if (msg->numv == last_refused_numv && msg->id == last_refused_id )
        return; 

    last_refused_numv = msg->numv;
    last_refused_id = msg->id;

    printf("%s[REFUS ENCHÈRE]%s L'enchère a été %sREFUSÉE%s pour cause de prix trop bas !\n", COLOR_RED, COLOR_RESET, COLOR_RED, COLOR_RESET);
    printf("%s→ Vente #%s%u%s | Offre : %s%u%s | Pair demandeur : %s%u%s\n",
           COLOR_CYAN, COLOR_YELLOW, msg->numv, COLOR_CYAN,
           COLOR_LIGHTGREEN, msg->prix, COLOR_CYAN,
           COLOR_YELLOW, msg->id, COLOR_RESET);
    printf("%s----------------------------------------%s\n", COLOR_RED, COLOR_RESET);
}
void handle_join_request(int udp_sock, PeerList *peers, struct sockaddr_in6 *sender_addr)
{

    printf("%s[INFO]%s Nouvelle demande de connexion reçue.\n", COLOR_CYAN, COLOR_RESET);
    message *response = rejoindre_enchere_mess3(
        peers->self_id,
        peers->peers[0].ip,
        peers->peers[0].port);
    // Devrai envoyer en TCP pas UDP
    if (send_message_udp_unicast(udp_sock, response, sender_addr) != 0)
    {
        fprintf(stderr, "%s[ERREUR]%s Erreur lors de l'envoi de la réponse UDP\n", COLOR_RED, COLOR_RESET);
    }
}

void handle_sale_init(sale *sales_list, int *cmpt_sale, message *msg)
{
    printf("\n%s[VENTE]%s Nouvelle vente initialisée par le pair %s%u%s\n", COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, msg->id, COLOR_RESET);
    printf("    → Vente #%s%u%s | Prix de départ : %s%u%s\n\n", COLOR_LIGHTGREEN, msg->numv, COLOR_RESET, COLOR_LIGHTGREEN, msg->prix, COLOR_RESET);
    sale_init(&sales_list[*cmpt_sale], msg->numv, msg->prix, msg->id);
    (*cmpt_sale)++;
}
// CODE 9
int handle_bid(PeerList *peers, sale *sales_list, int *vente, time_t *last_enchere, int *cmpt_sale, message *msg)
{
    if (*cmpt_sale == 0)
    {
        sale_init(&sales_list[*cmpt_sale], msg->numv, msg->prix, 0);
        *cmpt_sale = *cmpt_sale + 1;
    }
    int i = find_sale(msg->numv, sales_list, *cmpt_sale);

    if (peers->self_id == sales_list[i].superviseur_id)
    {
        *vente = 2;
        *last_enchere = time(NULL);
    }
    printf("\n%s[INFO]%s Offre d'enchère reçue\n", COLOR_CYAN, COLOR_RESET);
    printf("    → Vente #%u | Offre : %u | Par pair : %u\n\n", msg->numv, msg->prix, msg->id);
    return 1;
}

// RECEPTION CODE 10
void handle_new_bid(PeerList *peers, sale *sales_list, int *cmpt_sale, message *msg, int udp_sock, char *buffer, struct sockaddr_in6 sender_addr)
{
    int i = find_sale(msg->numv, sales_list, *cmpt_sale);
    if (i < 0)
    {
        return;
    }
    if (*cmpt_sale == 0)
    {
        sale_init(&sales_list[*cmpt_sale], msg->numv, msg->prix, 0);
        *cmpt_sale = *cmpt_sale + 1;
    }
    if (msg->id != peers->self_id)
    {
        printf("\n%s[SUCCÈS]%s Nouvelle enchère reçue\n", COLOR_GREEN, COLOR_RESET);
        printf("    → Vente #%u | Offre : %u | Par pair : %u\n\n", msg->numv, msg->prix, msg->id);
    }
    if (peers->peers[0].id != sales_list[i].superviseur_id)
    {
        char *sig = calloc(SIG_SIZE, sizeof(char));
        message *msg_peer = processus_consensus_mess1(peers->self_id, buffer, sig);
        send_message_udp_unicast(udp_sock, msg_peer, &sender_addr);
        free_message(msg_peer);
        free(sig);
    }
    if (i >= 0)
    {
        sale_update(&sales_list[i], msg->prix, msg->id);
        printf("%s[Mise à jour]%s Vente #%u\n", COLOR_YELLOW, COLOR_RESET, sales_list[i].num_vente);
        printf("    → Prix courant : %u | Enchérisseur en tête : %u\n\n", sales_list[i].prix_courant, sales_list[i].encherisseur_en_tete);
    }
}

void handle_closure_announce(message *msg)
{
    printf("\n%s[ANNONCE CLÔTURE]%s Vente #%u : clôture dans 30 secondes.\n\n", COLOR_MAGENTA, COLOR_RESET, msg->numv);
}

void handle_closure(PeerList *peers, message *msg, char *buffer, struct sockaddr_in6 sender_addr, int udp_sock, sale *sales_list, int cmpt)
{
    int i = find_sale(msg->numv, sales_list, cmpt);
    if (i >= 0)
    {
        sale_close(&sales_list[i]);
    }
    if (msg->id == peers->self_id)
    {
        printf("\n%s[GAGNÉ]%s Félicitations ! Vous avez remporté l'enchère pour la vente #%u !\n", COLOR_YELLOW, COLOR_RESET, msg->numv);
    }
    else
    {
        printf("\n%s[PERDU]%s Vous n'avez pas remporté l'enchère pour la vente #%u.\n", COLOR_PINK, COLOR_RESET, msg->numv);
    }
    if (peers->peers[0].id != sales_list[i].superviseur_id)
    {
        char *sig = calloc(SIG_SIZE, sizeof(char));
        message *msg_peer = processus_consensus_mess1(peers->self_id, buffer, sig);
        send_message_udp_unicast(udp_sock, msg_peer, &sender_addr);
        free_message(msg_peer);
        free(sig);
    }
    printf("%s[CLÔTURE]%s Vente #%u clôturée.\n\n", COLOR_MAGENTA, COLOR_RESET, msg->numv);
}

void handle_peer_departure(PeerList *peers, message *msg, char *buffer, struct sockaddr_in6 sender_addr, int udp_sock, sale *sales_list, int cmpt)
{
    int id = msg->id;
    sale *vente = is_superv(id, sales_list, cmpt);
    if (vente != NULL)
    {
        sale_close(vente);
    }
    if (peer_list_remove(peers, id) < 0)
    {
        perror("peer_list_remove");
        return;
    }
    if (peers->peers[0].id != msg->id)
    {
        char *sig = calloc(SIG_SIZE, sizeof(char));
        message *msg_peer = processus_consensus_mess1(peers->self_id, buffer, sig);
        send_message_udp_unicast(udp_sock, msg_peer, &sender_addr);
        free_message(msg_peer);
        free(sig);
    }
    printf("%s[DÉPART]%s Le pair %s%u%s a quitté le réseau.\n", COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, id, COLOR_RESET);
}

int handle_udp_liaison(int udp_sock, PeerList *peers)
{
    message *msg = malloc(sizeof(message));
    initialize_message(msg);
    struct sockaddr_in6 sender_addr;
    socklen_t addr_len = sizeof(sender_addr);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(udp_sock, &fds);

    struct timeval tv = {5, 0};

    int ready = select(udp_sock + 1, &fds, NULL, NULL, &tv);
    if (ready <= 0)
    {
        return -1;
    }

    char buffer[MTU];
    int n = recvfrom(udp_sock, buffer, MTU, 0, (struct sockaddr *)&sender_addr, &addr_len);
    if (n < 0)
    {
        perror("recvfrom failed");
        return -1;
    }

    int r = 0;
    msg = buf2struct(buffer, &r);

    if (msg->code == 3)
    {
        handle_join_request(udp_sock, peers, &sender_addr);
    }
    free_message(msg);
    return 0;
}

void handle_disparition(PeerList *peers, message **msg, char *buffer, struct sockaddr_in6 sender_addr, int udp_sock)
{
    if (peers->self_id != msg[0]->id)
    {
        sleep(1);
        char *sig = calloc(SIG_SIZE, sizeof(char));
        message *msg_peer = processus_consensus_mess1(peers->self_id, buffer, sig);
        send_message_udp_unicast(udp_sock, msg_peer, &sender_addr);
        free(sig);
        free_message(msg_peer);

        for (int i = 1; i <= msg[0]->nb; i++)
        {
            uint16_t id_disparu = msg[i]->id;
            peer_list_remove(peers, id_disparu);
        }
    }
    printf("%s[DÉPART]%s Le pair %s%u%s a disparu du réseau.\n", COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, msg[0]->nb, COLOR_RESET);
}