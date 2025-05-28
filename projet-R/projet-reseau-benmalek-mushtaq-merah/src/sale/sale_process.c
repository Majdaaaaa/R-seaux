#include "../headers/network.h"
#include "../headers/peers.h"
#include "../headers/struct_mess.h"
#include "../headers/structs/config.h"
#include "../headers/messages.h"
#include "../headers/sales.h"
#include <../headers/structs/sale.h>
#include "../headers/sale_process.h"
#include "../headers/utils.h"
#include "../headers/consensus.h"

#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <errno.h>
#include <time.h>

static int consensus_en_cours = 0; // 0 = non, 1 = oui
static uint32_t vente_consensus = 0;

void demander_prix_numvente(uint32_t info[2], sale *sales_list, int cmpt_sale)
{
    printf("\n================== Ventes actives ==================\n");
    int found = 0;
    for (int i = 0; i < cmpt_sale; i++)
    {
        if (sale_is_active(sales_list[i]))
        {
            printf("%s  - Vente #%u%s\n", COLOR_YELLOW, sales_list[i].num_vente, COLOR_RESET);
            printf("      Prix courant : %u\n", sales_list[i].prix_courant);
            printf("      ID dernier enchérisseur : %u\n", sales_list[i].encherisseur_en_tete);
            found = 1;
        }
    }
    if (!found)
    {
        printf("%sAucune vente active disponible.%s\n", COLOR_RED, COLOR_RESET);
    }
    printf("====================================================\n\n");
    int prix_valide = 0, numvente_valide = 0;
    while (!prix_valide)
    {
        printf("%sEntrez un prix : %s", COLOR_GREEN, COLOR_RESET);
        if (scanf("%u", &info[0]) != 1)
        {
            printf("%sEntrée invalide. Veuillez entrer un nombre entier.%s\n", COLOR_RED, COLOR_RESET);
            while (getchar() != '\n')
                ;
            prix_valide = 0;
        }
        else
        {
            prix_valide = 1;
        }
    }
    while (!numvente_valide)
    {

        printf("%sEntrez le numéro de vente : %s", COLOR_GREEN, COLOR_RESET);
        if (scanf("%u", &info[1]) != 1)
        {
            printf("%sEntrée invalide. Veuillez entrer un nombre entier.%s\n", COLOR_RED, COLOR_RESET);
            while (getchar() != '\n')
                ;
            numvente_valide = 0;
        }
        else
        {
            numvente_valide = 1;
        }
    }
}

void afficher_erreur_commande()
{
    printf("\n%s[ERREUR]%s Commande non reconnue. Essayez \"init vente\" ou \"encherir\" ou \"pairs\" ou \"exit\".\n\n", COLOR_RED, COLOR_RESET);
}

int inputUserSale(int res, sale *sales_list, int *cmpt_sale, int *vente, int udp_sock, PeerList *peers, time_t *last_enchere)
{
    switch (res)
    {
    case 1:
        printf("\n%s[INFO]%s Initialisation d'une vente...\n", COLOR_CYAN, COLOR_RESET);
        int prix;
        int prix_v = 0;
        while (!prix_v)
        {
            printf("%sEntrez un prix de base : %s", COLOR_LIGHTGREEN, COLOR_RESET);
            if (scanf("%u", &prix) != 1)
            {
                printf("%sEntrée invalide. Veuillez entrer un nombre entier.%s\n", COLOR_RED, COLOR_RESET);
                while (getchar() != '\n')
                    ;
                prix_v = 0;
            }
            else
            {
                prix_v = 1;
            }
        }
        sale_start(udp_sock, peers, sales_list, cmpt_sale, prix);
        *vente = 2;
        *last_enchere = time(NULL);
        printf("\n%s[SUCCÈS]%s Vente #%u initialisée avec succès !\n\n", COLOR_GREEN, COLOR_RESET, sales_list[(*cmpt_sale) - 1].num_vente);
        break;
    case 2:
    {
        uint32_t info[2] = {0};
        demander_prix_numvente(info, sales_list, *cmpt_sale);
        if (peers->peers[0].vente_supervisee == info[1])
        {
            printf("\n%s[REFUS]%s Le superviseur ne peut pas enchérir sur sa propre vente.\n\n", COLOR_MAGENTA, COLOR_RESET);
            return -1;
        }
        if (encherir(info, udp_sock, peers, sales_list, *cmpt_sale) == NULL)
        {
            printf("\n%s[ERREUR]%s Impossible de placer l'enchère.\n\n", COLOR_RED, COLOR_RESET);
            return -1;
        }
        else
        {
            printf("\n%s[SUCCÈS]%s Enchère envoyée !\n\n", COLOR_GREEN, COLOR_RESET);
        }
        break;
    }
    default:
        afficher_erreur_commande();
        break;
    }
    return 0;
}

int sale_start(int udpsock, PeerList *list, sale *sale_list, int *cmpt_sale, int prixb)
{
    Peer *super = peer_list_find(list, list->self_id);
    super->cpt += 1;
    uint32_t numv = (super->id << 16) | (super->cpt & 0xFFFF);
    super->vente_supervisee = numv;
    sale_init(&sale_list[*cmpt_sale], numv, prixb, super->id);
    (*cmpt_sale)++;
    message *m = initier_vente(super->id, numv, prixb);
    char buf[MTU];
    int r = struct2buf(m, buf);
    int s = send_message_udp_multicast(udpsock, buf, r, ENCHERE_IP, ENCHERE_PORT);
    free_message(m);
    return s;
}

message *encherir(uint32_t info[2], int udp_sock, PeerList *list, sale *sales_list, int cmpt_sale)
{
    int i = find_sale(info[1], sales_list, cmpt_sale);
    if (i < 0 || !sale_is_active(sales_list[i]))
    {
        printf("%s[ERREUR]%s Vente annulée, finie ou inexistante.\n", COLOR_RED, COLOR_RESET);
        return NULL;
    }
    message *m = encherir_vente(list->self_id, info[1], info[0]);
    char buf[MTU];
    int r = struct2buf(m, buf);
    int s = send_message_udp_multicast(udp_sock, buf, r, ENCHERE_IP, ENCHERE_PORT);
    if (s < 0)
    {
        perror("send udp");
        free_message(m);
        return NULL;
    }
    free_message(m);
    return m;
}

int verifier_enchere(message *msg_copy, sale *sales_list, time_t *last_enchere, uint16_t me_id, int udp_sock, int cmpt_sale, int nombre_peer, PeerList *peers)
{
    int i = find_sale(msg_copy->numv, sales_list, cmpt_sale);
    if (i == -1)
    {
        printf("\n%s[ERREUR]%s Vente non trouvée.\n\n", COLOR_RED, COLOR_RESET);
        return -1;
    }
    if (sales_list[i].superviseur_id == me_id && msg_copy->code == 9)
    {

        if (sales_list[i].prix_courant >= msg_copy->prix)
        {
            message *m = rejet_enchere(15, msg_copy->id, msg_copy->numv, msg_copy->prix);
            char buffer[MTU];
            int r = struct2buf(m, buffer);
            int s = send_message_udp_multicast(udp_sock, buffer, r, ENCHERE_IP, ENCHERE_PORT);
            if (s < 0)
            {
                perror("send dans verifier_enchere");
                free_message(m);
                return -1;
            }
            free_message(m);
            return 0;
        }

        *last_enchere = time(NULL);
        consensus_en_cours = 1;
        vente_consensus = msg_copy->numv;
        encherir_superviseur(msg_copy);
        message *final[1] = {msg_copy};
        int c = consensus(udp_sock, final, 1, nombre_peer, peers);
        if (c < 0)
        {
            printf("%s[ERREUR]%s Consensus échoué.\n\n", COLOR_RED, COLOR_RESET);
            consensus_en_cours = 0;
            return -1;
        }
        else
        {
            printf("%s[CONSENSUS]%s Consensus validé.\n\n", COLOR_BLUE, COLOR_RESET);
        }

        consensus_en_cours = 0;
        printf("\n%s[SUCCÈS]%s Nouvelle enchère acceptée pour la vente #%u.\n", COLOR_GREEN, COLOR_RESET, msg_copy->numv);
        sale_update(&sales_list[i], msg_copy->prix, msg_copy->id);
        printf("%s[Mise à jour]%s Vente #%u | Prix courant : %s%u%s | Enchérisseur : %s%u%s\n\n",
               COLOR_YELLOW, COLOR_RESET, sales_list[i].num_vente,
               COLOR_LIGHTGREEN, sales_list[i].prix_courant, COLOR_RESET,
               COLOR_LIGHTGREEN, sales_list[i].encherisseur_en_tete, COLOR_RESET);
    }
    return 0;
}

int alerte_fin_enchere(message *msg_copy, sale *sales_list, int udp_sock, time_t *last_enchere, int cmpt_sale)
{
    int i = 0;
    if (msg_copy != NULL)
    {
        i = find_sale(msg_copy->numv, sales_list, cmpt_sale);
        if (i < 0)
        {
            printf("\n%s[ERREUR]%s Vente non trouvée.\n\n", COLOR_RED, COLOR_RESET);
            return -1;
        }
    }
    message *m = finalisation_mess1(sales_list[i].superviseur_id, sales_list[i].num_vente, sales_list[i].prix_courant);
    char buffer[MTU];
    int len = struct2buf(m, buffer);
    if (len < 0)
        return -1;
    int s = send_message_udp_multicast(udp_sock, buffer, len, ENCHERE_IP, ENCHERE_PORT);
    if (s < 0)
        return -1;
    *last_enchere = time(NULL);

    printf("\n%s[ALERTE]%s 30 secondes sans nouvelle enchère sur la vente #%u.\n\n", COLOR_MAGENTA, COLOR_RESET, sales_list[i].num_vente);
    free_message(m);
    return 0;
}

void finalisation_enchere(message *msg_copy, sale *sales_list, int udp_sock, int cmpt_sale, int nombre_peer, PeerList *peers)
{
    int i = -1;
    if (msg_copy != NULL)
    {
        i = find_sale(msg_copy->numv, sales_list, cmpt_sale);
        if (i < 0 || !sale_is_active(sales_list[i]))
        {
            for (int j = 0; j < cmpt_sale; j++)
            {
                if (sale_is_active(sales_list[j]))
                {
                    i = j;
                    break;
                }
            }
        }
    }
    else
    {
        for (int j = 0; j < cmpt_sale; j++)
        {
            if (sale_is_active(sales_list[j]))
            {
                i = j;
                break;
            }
        }
    }
    if (i < 0 || !sale_is_active(sales_list[i]))
    {
        printf("\n%s[ERREUR]%s Vente non trouvée ou déjà clôturée, impossible de clôturer.\n\n", COLOR_RED, COLOR_RESET);
        return;
    }
    uint16_t id = (sales_list[i].encherisseur_en_tete != 0) ? sales_list[i].encherisseur_en_tete : sales_list[i].superviseur_id;
    message *m = finalisation_mess2(id, sales_list[i].num_vente, sales_list[i].prix_courant);
    printf("\n%s[ALERTE]%s Aucune enchère reçue depuis 60 secondes. Clôture imminente...\n", COLOR_MAGENTA, COLOR_RESET);
    for (int j = 3; j > 0; j--)
    {
        printf("  Clôture dans %d...\n", j);
        sleep(1);
    }
    printf("\n%s[INFO]%s Lancement du consensus de clôture...\n", COLOR_CYAN, COLOR_RESET);
    message *final[1] = {m};
    int c = consensus(udp_sock, final, 1, nombre_peer, peers);
    if (c < 0)
        printf("\n%s[ERREUR]%s Consensus de clôture échoué.\n\n", COLOR_RED, COLOR_RESET);
    else
        printf("\n%s[SUCCÈS]%s Consensus de clôture validé.\n\n", COLOR_GREEN, COLOR_RESET);

    sale_close(&sales_list[i]);
    printf("\n%s[INFO]%s Fin de la vente #%u | Dernier prix : %s%u%s | Gagnant : %s%u%s\n",
           COLOR_CYAN, COLOR_RESET, sales_list[i].num_vente,
           COLOR_LIGHTGREEN, sales_list[i].prix_courant, COLOR_RESET,
           COLOR_LIGHTGREEN, id, COLOR_RESET);
    printf("%s[CLÔTURE]%s Vente #%u clôturée.\n\n", COLOR_MAGENTA, COLOR_RESET, sales_list[i].num_vente);
    free_message(m);
}