#include <openssl/evp.h>
#include "message.h"
#include <messages.h>
#include "../headers/structs/config.h"
#include "../headers/messages.h"
#include "../headers/network.h"
// #define MAX 1024
#include <peer.h>
#include <peers.h>
#include <network.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>

int disparition_peer_lambda(int udp_sock, int id_sup, Peer *disparus, int *dispa_cmp, int total_peer, PeerList *peers);

bool pas_encore_rep(int *tab, int id, ssize_t size)
{
    for (ssize_t i = 0; i < size; i++)
    {
        if (tab[i] == id)
        {
            return false;
        }
    }
    return true;
}

bool update_reponse(message *m_superviseur, char *buffer, message *recu, Peer *ont_rep, int *rep_cmp)
{
    bool condition = recu->code == 1 && memcmp(buffer, recu->mess, m_superviseur->lmess) == 0;
    for (int i = 0; i < *rep_cmp; i++)
    {
        if (recu->id == ont_rep[i].id)
        {
            return false;
        }
    }
    return condition;
}

struct sockaddr_in6 initialise_addr(Peer p)
{
    struct sockaddr_in6 addr_peer;
    memset(&addr_peer, 0, sizeof(addr_peer));
    addr_peer.sin6_family = AF_INET6;
    inet_pton(AF_INET6, (const char *)p.ip, &addr_peer.sin6_addr);
    addr_peer.sin6_port = htons(p.port);
    return addr_peer;
}

int relance(int udp_sock, message *m, Peer p, message *recu)
{
    struct sockaddr_in6 addr_peer = initialise_addr(p);
    send_message_udp_unicast(udp_sock, m, &addr_peer);
    int r = receive_message_udp_and_extract_adr(udp_sock, &recu, NULL, NULL, 5);
    if (r == 0)
    {
        send_message_udp_unicast(udp_sock, m, &addr_peer);
        r = receive_message_udp_and_extract_adr(udp_sock, &recu, NULL, NULL, 5);
    }
    return r;
}

void section_g(int udp_sock, Peer *doit_rep, int pas_rep_cmp, Peer *ont_rep, int *rep_cmp, message *a_envoyer, Peer *disparus, int *dispa_cmp)
{
    message *recu = malloc(sizeof(message));
    if (recu == NULL)
    {
        perror("malloc");
        return;
    }
    for (int i = 0; i < pas_rep_cmp; i++)
    {
        memset(&recu, 0, sizeof(recu));
        int rep = relance(udp_sock, a_envoyer, doit_rep[i], recu);
        if (rep == 0)
        {
            // section H
            disparus[*dispa_cmp] = doit_rep[i];
            *dispa_cmp = *dispa_cmp + 1;
        }
        else
        {
            bool condition = update_reponse(a_envoyer, recu->mess, recu, ont_rep, rep_cmp);
            if (condition == true)
            {
                ont_rep[*rep_cmp] = doit_rep[i];
                *rep_cmp = *rep_cmp + 1;
            }
            else
            {
                continue;
            }
        }
    }
    free_message(recu);
}

int consensus(int udp_sock, message **m_pair_Q, int nb_mess, int total_Peer, PeerList *peers)
{
    if (total_Peer == 1)
    {
        return 0;
    }
    // Convert message to buffer
    char buffer[MTU];
    int len;
    if (nb_mess == 1)
    {
        len = struct2buf(m_pair_Q[0], buffer);
    }
    else
    {
        len = structs2buf(m_pair_Q, nb_mess, buffer);
    }
    if (len < 0)
    {
        return -1;
    }

    ssize_t s = send_message_udp_multicast(udp_sock, buffer, len, ENCHERE_IP, ENCHERE_PORT);
    int *cmp_rep = malloc(sizeof(int));
    if (cmp_rep == NULL)
    {
        perror("malloc");
        return -1;
    }
    *cmp_rep = 0;
    int total = 0;
    message *recu = malloc(sizeof(message));
    if (recu == NULL)
    {
        perror("malloc");
        return -1;
    }
    int *ids = malloc((total_Peer - 1) * sizeof(int));
    if (ids == NULL)
    {
        perror("malloc");
        return -1;
    }
    while (total < total_Peer - 1)
    {
        memset(&recu, 0, sizeof(recu));
        int r = receive_message_udp_and_extract_adr(udp_sock, &recu, NULL, NULL, 5);
        if (r == 0)
        {
            Peer *doit_rep = malloc(sizeof(Peer) * total_Peer);
            if (doit_rep == NULL)
            {
                perror("malloc");
                return -1;
            }
            int pas_rep_cmp = 0;
            for (size_t i = 0; i < peers->count; i++)
            {
                if (pas_encore_rep(ids, peers->peers[i].id, *cmp_rep) == true && peers->peers[i].id != peers->self_id)
                {
                    doit_rep[pas_rep_cmp] = peers->peers[i];
                    pas_rep_cmp++;
                }
            }
            Peer *ont_rep = malloc(sizeof(Peer) * total_Peer);
            if (ont_rep == NULL)
            {
                perror("malloc");
                free(doit_rep);
                free(cmp_rep);
                free_message(recu);
                free(ids);
                return -1;
            }
            int *rep_peer = malloc(sizeof(int));
            if (rep_peer == NULL)
            {
                perror("malloc");
                free(doit_rep);
                free(ont_rep);
                free(cmp_rep);
                free_message(recu);
                free(ids);
                return -1;
            }
            *rep_peer = 0;
            Peer *disparu = malloc(sizeof(Peer) * total_Peer);
            if (disparu == NULL)
            {
                perror("malloc");
                free(doit_rep);
                free(ont_rep);
                free(rep_peer);
                free(cmp_rep);
                free_message(recu);
                free(ids);
                return -1;
            }
            int *dispa_cmp = malloc(sizeof(int));
            if (dispa_cmp == NULL)
            {
                perror("malloc");
                free(doit_rep);
                free(ont_rep);
                free(rep_peer);
                free(disparu);
                free(cmp_rep);
                free_message(recu);
                free(ids);
                return -1;
            }
            *dispa_cmp = 0;
            section_g(udp_sock, doit_rep, pas_rep_cmp, ont_rep, rep_peer, m_pair_Q[0], disparu, dispa_cmp);

            for (int i = 0; i < *rep_peer; i++)
            {

                ids[*cmp_rep] = ont_rep[i].id;
                *cmp_rep = *cmp_rep + 1;
            }
            if (*dispa_cmp > 0)
            {
                disparition_peer_lambda(udp_sock, peers->self_id, disparu, dispa_cmp, total_Peer, peers);
                total += *dispa_cmp;
            }
        }
        else if (r == -1)
        {

            continue;
        }
        else
        {
            // verifier le message
            bool res = recu->code == 1 && memcmp(buffer, recu->mess, m_pair_Q[0]->lmess) == 0 && pas_encore_rep(ids, recu->id, *cmp_rep); //&& strcmp(m_pair_Q->sig,recu->sig)
            if (res == true)
            {
                ids[*cmp_rep] = recu->id;
                *cmp_rep = *cmp_rep + 1;
                total++;
            }
            else if (recu->code == 9)
            {
                message *m = rejet_enchere(14, recu->id, recu->numv, recu->prix);
                char buf_rejet[MTU];
                int r = struct2buf(m, buf_rejet);

                int s = send_message_udp_multicast(udp_sock, buf_rejet, r, ENCHERE_IP, ENCHERE_PORT);
                if (s < 0)
                {
                    perror("send");
                    free_message(m);
                    free(cmp_rep);
                    free_message(recu);
                    free(ids);
                    return -1;
                }
                free_message(m);
                continue;
            }
            else
            {
                Peer *doit_rep = malloc(sizeof(Peer) * total_Peer);
                if (doit_rep == NULL)
                {
                    perror("malloc");
                    return -1;
                }
                int pas_rep_cmp = 0;
                for (size_t i = 0; i < peers->count; i++)
                {
                    if (pas_encore_rep(ids, peers->peers[i].id, *cmp_rep) == true && peers->peers[i].id != peers->self_id)
                    {
                        doit_rep[pas_rep_cmp] = peers->peers[i];
                        pas_rep_cmp++;
                    }
                }
                Peer *ont_rep = malloc(sizeof(Peer) * total_Peer);
                int *rep_peer = malloc(sizeof(int));
                *rep_peer = 0;
                Peer *disparu = malloc(sizeof(Peer) * total_Peer);
                int *dispa_cmp = malloc(sizeof(int));
                *dispa_cmp = 0;

                section_g(udp_sock, doit_rep, pas_rep_cmp, ont_rep, rep_peer, m_pair_Q[0], disparu, dispa_cmp);

                for (int i = 0; i < *rep_peer; i++)
                {

                    ids[*cmp_rep] = ont_rep[i].id;
                    *cmp_rep = *cmp_rep + 1;
                }
                if (*dispa_cmp > 0)
                {
                    disparition_peer_lambda(udp_sock, peers->self_id, disparu, dispa_cmp, total_Peer, peers);
                    total += *dispa_cmp;
                }
            }
        }
    }

    // on a reçu le message de tt le monde
    message **a_envoyer = processus_consensus_mess4(*cmp_rep, m_pair_Q[0]->mess, m_pair_Q[0]->sig, m_pair_Q[0]->id);
    char buf[MTU];
    int t = structs2buf(a_envoyer, *cmp_rep, buf);
    if (t < 0)
    {
        free_messages(a_envoyer, *cmp_rep);
        free(cmp_rep);
        free_message(recu);
        free(ids);
        return -1;
    }
    s = send_message_udp_multicast(udp_sock, buf, t, ENCHERE_IP, ENCHERE_PORT);
    if (s < 0)
    {
        free_messages(a_envoyer, *cmp_rep);
        free(cmp_rep);
        free_message(recu);
        free(ids);
        return -1;
    }
    return 0;
}

int disparition_peer_lambda(int udp_sock, int id_sup, Peer *disparus, int *dispa_cmp, int total_peer, PeerList *peers)
{
    message **m = disparition_pair(id_sup, *dispa_cmp, disparus, dispa_cmp);
    char buffer[MTU];
    int c = *dispa_cmp + 1;
    int len = structs2buf(m, c, buffer);
    if (len < 0)
    {
        printf("problème dans le len\n");
        return -1;
    }
    int rep_attendu = total_peer - *dispa_cmp;
    int cc = consensus(udp_sock, m, c, rep_attendu, peers);
    if (cc > 0)
    {
        for (int i = 0; i < *dispa_cmp; i++)
        {
            peer_list_remove(peers, disparus[i].id);
        }
    }
    return cc;
}

// todo faire une fonction pr que tt les pairs verif si tt le monde à signer

// ?--- pr la partie 4
// //* engendrer une paire de clés (privée,publique) au format ED25519 pour un pair
// int create_key(EVP_PKEY *pkey){
//     EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
//     if (pctx == NULL) {
//         return -1;
//     }
//     if (EVP_PKEY_keygen_init(pctx) <=0){
//         return -1;
//     }
//     // EVP_PKEY *pkey = NULL;
//     if (EVP_PKEY_generate(pctx,&pkey) <= 0){
//         return -1;
//     }
// }
// //TODO ne pas oublié de libérer la mémoire avc
// /**
//  * EVP_PKEY(pkey);
//  * EVP_PKEY_CTX_free(pctx);
//  */
// //* pour valider un message un pair signe le message avec sa clé privée
// int valid_message(EVP_PKEY *pkey, message *msg){
//     EVP_MD_CTX *mdctx;
//     if ((mdctx = EVP_MD_CTX_create()) == NULL){
//         return -1;
//     }
//     if (EVP_DigestSignInit(mdctx,NULL,NULL,NULL,pkey) != 1){
//         return -1;
//     }
//     // ? on doit récup le contexte de la clé EVP ou pas si oui mettre 2ème paramètre EVP_PKEY_CTX **pctx
//     size_t slen;
//     unsigned char *sig;
//     char buf[MAX];
//     int r = struct2buf(msg,buf);
//     if (EVP_DigestSign(mdctx,NULL,&slen,buf,strlen(buf)) != 1){ // ? pas sûre de faire struct 2 buf
//         return -1;
//     }
//     if (!(sig = malloc(sizeof(*sig)*slen))){
//         return -1;
//     }
//     if (EVP_DigestSign(mdctx,sig,&slen,buf,strlen(buf)) != 1){//TODO maybe a revoir le buf
//         return -1;
//     }
//     /*
//     TODO liberer les vairables alloués qd on utilise plus
//     EVP_MD_CTX_free(mdctx);
//     free(sig);
//     */
// }
// //* verifier un message signé par un autre pair
// int check_message(message *msg){
//     EVP_MD_CTX *mdctx;
//     if ((mdctx = EVP_PD_CTX_create()) == NULL){
//         return -1;
//     }
//     if (EVP_DigestVerifyInit(mdctx,NULL,NULL,pbkey) != 1){
//         return -1;
//     }
//     if (EVP_DigestVerify(mdctx,))
// }
// //* verifier qu'un message est signé par tous les autres pairs
// int sign_check(message *msg){
// }
// bool verifier_msg(message *original,message *recu){
//     return recu->code == 1 && strcmp(original->mess,recu->mess);
// }