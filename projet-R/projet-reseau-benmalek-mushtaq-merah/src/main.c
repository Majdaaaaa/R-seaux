#include "headers/peers.h"
#include "headers/structs/config.h"
#include "headers/network.h"
#include "headers/struct_mess.h"
#include "headers/sale_process.h"
#include "headers/udp_message_recv.h"
#include "headers/messages.h"
#include "headers/join.h"
#include "headers/sales.h"
#include "headers/utils.h"
#include "headers/consensus.h"

#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

#define MAX_SALES 100

int inputUser(int stdin_fd)
{
    char buf[256];
    ssize_t n = read(stdin_fd, buf, sizeof(buf) - 1);
    if (n > 0)
    {
        buf[n] = '\0';
        char *newline = strchr(buf, '\n');
        if (newline)
            *newline = '\0';

        if (strstr(buf, "init vente") != NULL)
            return 1;
        else if (strstr(buf, "encherir") != NULL)
            return 2;
        else if (strstr(buf, "pairs") != NULL)
            return 3;
        else if (strstr(buf, "exit") != NULL)
            return 4;
        else if (strstr(buf, "ventes") != NULL)
            return 5;
        else
        {
            printf("%s[ERREUR]%s Commande non reconnue. Essayez \"init vente\" ou \"encherir\".\n", COLOR_RED, COLOR_RESET);
            return -1;
        }
    }
    return 0;
}

int main_event_loop(int udp_sock, int tcp_sock, int udp_enchere, PeerList *peers)
{
    fd_set read_fds, write_fds;
    int stdin_fd = STDIN_FILENO;
    int max_fd = udp_sock;
    if (tcp_sock > max_fd)
        max_fd = tcp_sock;
    if (udp_enchere > max_fd)
        max_fd = udp_enchere;
    if (stdin_fd > max_fd)
        max_fd = stdin_fd;
    max_fd += 1;
    sale sales_list[MAX_SALES] = {0};
    int cmpt_sale = 0, vente = -1;
    message *msg = NULL, *msg_copy = NULL;
    time_t last_enchere = 0;
    Peer *me = peer_list_find(peers, peers->self_id);

    printf("%sBienvenue dans AuctionP2P !%s\n", COLOR_CYAN, COLOR_RESET);
    printf("Commandes disponibles :\n");
    printf("  %sinit vente%s   - Démarrer une nouvelle vente\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sencherir%s    - Placer une enchère\n", COLOR_GREEN, COLOR_RESET);
    printf("  %spairs%s       - Afficher la liste des pairs\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sexit%s        - Quitter\n\n", COLOR_GREEN, COLOR_RESET);

    while (1)
    {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(udp_sock, &read_fds);
        FD_SET(tcp_sock, &read_fds);
        FD_SET(stdin_fd, &read_fds);
        FD_SET(udp_sock, &write_fds);
        FD_SET(udp_enchere, &write_fds);
        FD_SET(udp_enchere, &read_fds);

        int ready = select(max_fd, &read_fds, &write_fds, NULL, NULL);
        if (ready < 0)
        {
            perror("select failed");
            break;
        }

        if (FD_ISSET(tcp_sock, &read_fds))
            handle_incoming_tcp(tcp_sock, peers);

        if (FD_ISSET(stdin_fd, &read_fds))
        {
            printf("%s\n> Commande : %s", COLOR_YELLOW, COLOR_RESET);
            int i = inputUser(stdin_fd);
            if (i == 1 || i == 2)
            {
                printf("%s--------------------------------------------------%s\n", COLOR_BLUE, COLOR_RESET);
                inputUserSale(i, sales_list, &cmpt_sale, &vente, udp_enchere, peers, &last_enchere);
                printf("%s--------------------------------------------------%s\n", COLOR_BLUE, COLOR_RESET);
            }
            else if (i == 3)
            {
                printf("%sListe des pairs connectés :%s\n", COLOR_CYAN, COLOR_RESET);
                print_peer_list(peers);
            }
            else if (i == 4)
            {
                message *m = quitter_enchere(peers->self_id);
                message *final[1] = {m};
                int c = consensus(udp_enchere, final, 1, peers->count, peers);
                if (c < 0)
                {
                    printf("le consensus s'est mal passé pour exit\n");
                    break;
                }
                printf("%s[INFO]%s Déconnexion du réseau. À bientôt !\n", COLOR_CYAN, COLOR_RESET);
                break;
            }
            else if (i == 5)
            {
                printf("%sListe des ventes :%s\n", COLOR_CYAN, COLOR_RESET);
                for (int i = 0; i < cmpt_sale; i++)
                {
                    sale_print(&sales_list[i]);
                }
            }
        }

        if (FD_ISSET(udp_enchere, &read_fds))
        {
            msg = handle_udp_message(udp_enchere, peers, sales_list, &cmpt_sale, &vente, &last_enchere);
            if (!msg)
            {
                perror("udp_recv");
                break;
            }
            if (msg->code == 9)
            {
                if (msg_copy)
                    free(msg_copy);
                msg_copy = malloc(sizeof(message));
                memcpy(msg_copy, msg, sizeof(message));
            }
            else
            {
                msg = NULL;
            }
        }

        if (FD_ISSET(udp_sock, &read_fds))
        {
            if (handle_udp_liaison(udp_sock, peers) < 0)
            {
                return -1;
            }
        }

        if (FD_ISSET(udp_enchere, &write_fds))
        {
            if (vente == 2 && time(NULL) - last_enchere >= 40)
            {
                alerte_fin_enchere(msg_copy, sales_list, udp_enchere, &last_enchere, cmpt_sale);
                vente = 3;
                continue;
            }
            if (vente == 3 && time(NULL) - last_enchere >= 30)
            {
                finalisation_enchere(msg_copy, sales_list, udp_enchere, cmpt_sale, peers->count, peers);
                vente = -1;
                continue;
            }
            if (vente == 2 && msg_copy != NULL)
            {
                if (verifier_enchere(msg_copy, sales_list, &last_enchere, me->id, udp_enchere, cmpt_sale, peers->count, peers) < 0)
                    break;
                else
                    continue;
            }
        }
    }

    if (msg)
        free(msg);
    if (msg_copy)
        free(msg_copy);

    return 0;
}

int main()
{
    srand(time(NULL));

    PeerList *peers = peer_list_create(10);
    if (!peers)
    {
        fprintf(stderr, "%s[FAIL]%s Failed to create peer list\n", COLOR_RED, COLOR_RESET);
        return EXIT_FAILURE;
    }

    initialize_1st_peer_info(peers);

    if (set_own_ipv6_address(peers) < 0)
    {
        peer_list_destroy(peers);
        return EXIT_FAILURE;
    }

    print_peer_info(&peers->peers[0]);
    int udp_sock, tcp_sock, udp_enchere;
    if (initialize_sockets(&udp_sock, &tcp_sock, &udp_enchere) < 0)
    {
        peer_list_destroy(peers);
        return EXIT_FAILURE;
    }

    if (join_existing_network(peers, udp_sock) < 0)
    {
        printf("%s[FAIL]%s No existing network found. Creating new network.\n", COLOR_RED, COLOR_RESET);
        initialize_multicast_info(peers);
        printf("%s[INFO]%s Network initialized. Self ID: %d\n", COLOR_CYAN, COLOR_RESET, peers->self_id);
    }

    main_event_loop(udp_sock, tcp_sock, udp_enchere, peers);

    close_socket(udp_sock);
    close_socket(tcp_sock);
    close_socket(udp_enchere);
    peer_list_destroy(peers);

    return EXIT_SUCCESS;
}