#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

#include "../headers/struct_mess.h"

int struct2buf(message *m, char *buf)
{
    if (buf == NULL)
        return -1;
    // Vérifier la taille des champs avant copie
    if (m->lmess >= MESS_SIZE || m->lsig >= SIG_SIZE || strlen(m->cle) >= CLE_SIZE)
    {
        return -1;
    }

    int index = 0;

    // Code
    memcpy(buf + index, &m->code, sizeof(uint8_t));
    index += sizeof(uint8_t);

    // ID (convertir en big-endian)
    uint16_t id_be = htons(m->id);
    memcpy(buf + index, &id_be, sizeof(uint16_t));
    index += sizeof(uint16_t);

    // Longueur du message
    memcpy(buf + index, &m->lmess, sizeof(uint8_t));
    index += sizeof(uint8_t);

    // Longueur de la signature
    memcpy(buf + index, &m->lsig, sizeof(uint8_t));
    index += sizeof(uint8_t);

    // Message
    memcpy(buf + index, m->mess, m->lmess);
    index += m->lmess;

    // Signature
    memcpy(buf + index, m->sig, m->lsig);
    index += m->lsig;

    // Nombre de pairs
    uint16_t nb_be = htons(m->nb);
    memcpy(buf + index, &nb_be, sizeof(uint16_t));
    index += sizeof(uint16_t);

    // Numéro de la vente
    uint32_t numv_be = htonl(m->numv);
    memcpy(buf + index, &numv_be, sizeof(uint32_t));
    index += sizeof(uint32_t);

    // Prix de la vente
    uint32_t prix_be = htonl(m->prix);
    memcpy(buf + index, &prix_be, sizeof(uint32_t));
    index += sizeof(uint32_t);

    // Adresse IP
    memcpy(buf + index, m->ip, sizeof(m->ip));
    index += sizeof(m->ip);

    // Port
    uint16_t port_be = htons(m->port);
    memcpy(buf + index, &port_be, sizeof(uint16_t));
    index += sizeof(uint16_t);

    // Clé
    memcpy(buf + index, m->cle, strlen(m->cle));
    index += strlen(m->cle);

    // Ajouter \r\n à la fin du buffer pour TCP
    buf[index++] = '\r';
    buf[index++] = '\n';

    return index;
}

message *buf2struct(char *buffer, int *len)
{
    message *m = malloc(sizeof(message));
    if (m == NULL)
    {
        return NULL;
    }
    initialize_message(m);
    *len = 0;

    // Code
    memcpy(&m->code, buffer + *len, sizeof(uint8_t));
    *len += sizeof(uint8_t);

    // ID (convertir en little-endian)
    uint16_t id_be;
    memcpy(&id_be, buffer + *len, sizeof(uint16_t));
    m->id = ntohs(id_be);
    *len += sizeof(uint16_t);

    // Longueur du message
    memcpy(&m->lmess, buffer + *len, sizeof(uint8_t));
    *len += sizeof(uint8_t);

    // Longueur de la signature
    memcpy(&m->lsig, buffer + *len, sizeof(uint8_t));
    *len += sizeof(uint8_t);

    // Message
    memcpy(m->mess, buffer + *len, m->lmess);
    m->mess[m->lmess] = '\0';
    *len += m->lmess;

    // Signature
    memcpy(m->sig, buffer + *len, m->lsig);
    m->sig[m->lsig] = '\0';
    *len += m->lsig;

    // Nombre de pairs
    uint16_t nb_le;
    memcpy(&nb_le, buffer + *len, sizeof(uint16_t));
    m->nb = ntohs(nb_le);
    *len += sizeof(uint16_t);

    // Numéro de la vente
    uint32_t numv_le;
    memcpy(&numv_le, buffer + *len, sizeof(uint32_t));
    m->numv = ntohl(numv_le);
    *len += sizeof(uint32_t);

    // Prix de la vente
    uint32_t prix_le;
    memcpy(&prix_le, buffer + *len, sizeof(uint32_t));
    m->prix = ntohl(prix_le);
    *len += sizeof(uint32_t);

    // Adresse IP
    memcpy(m->ip, buffer + *len, sizeof(m->ip));
    *len += sizeof(m->ip);

    // Port
    uint16_t port_le;
    memcpy(&port_le, buffer + *len, sizeof(uint16_t));
    m->port = ntohs(port_le);
    *len += sizeof(uint16_t);

    // Clé - Ici on ne cherche plus nécessairement \r\n pour la fin
    char *end = strstr(buffer + *len, "\r\n");
    int remaining_len;

    if (end != NULL)
    {
        // Si on trouve \r\n, calculer la longueur jusqu'à ce délimiteur
        remaining_len = end - (buffer + *len);

        // S'assurer que la taille ne dépasse pas CLE_SIZE
        if (remaining_len >= CLE_SIZE)
            remaining_len = CLE_SIZE - 1;

        memcpy(m->cle, buffer + *len, remaining_len);
        m->cle[remaining_len] = '\0';
        *len += remaining_len;

        // Ignorer \r\n à la fin
        *len += 2;
    }
    else
    {
        // Si pas de \r\n (cas des messages intermédiaires), utiliser strlen
        remaining_len = strlen(buffer + *len);
        if (remaining_len >= CLE_SIZE)
            remaining_len = CLE_SIZE - 1;

        if (remaining_len > 0)
        {
            memcpy(m->cle, buffer + *len, remaining_len);
            m->cle[remaining_len] = '\0';
            *len += remaining_len;
        }
        else
        {
            m->cle[0] = '\0';
        }
    }

    if (m->lmess >= MESS_SIZE || m->lsig >= SIG_SIZE || strlen(m->cle) >= CLE_SIZE)
    {
        free(m);
        return NULL;
    }

    return m;
}

int structs2buf(message **msgs, int count, char *buffer)
{
    int total_size = 0;

    for (int i = 0; i < count; i++)
    {

        int msg_size = struct2buf(msgs[i], buffer + total_size);
        if (msg_size < 0)
        {
            return -1;
        }

        if (total_size + msg_size > MTU)
        {
            fprintf(stderr, "Erreur: le buffer est trop petit pour contenir tous les messages\n");
            return -1;
        }

        // Si ce n'est pas le dernier message, on retire \r\n
        if (i < count - 1)
        {
            total_size += msg_size - 2;
        }
        else
        {
            total_size += msg_size;
        }
    }

    return total_size;
}

message **buf2structs(char *buffer, size_t *count, size_t lenbuffer)
{
    *count = 0;
    size_t index = 0;
    size_t capacity = 4;

    message **msgs = malloc(capacity * sizeof(message *));
    if (!msgs)
        return NULL;

    while (index < lenbuffer)
    {
        if (*count >= capacity)
        {
            capacity *= 2;
            message **tmp = realloc(msgs, capacity * sizeof(message *));
            if (!tmp)
            {
                free_messages(msgs, *count);
                return NULL;
            }
            msgs = tmp;
        }

        // Extraire le message à partir du buffer
        int read;
        msgs[*count] = buf2struct(buffer + index, &read);

        // Vérifier si la désérialisation a fonctionné
        if (msgs[*count] == NULL || read <= 0)
        {
            fprintf(stderr, "Erreur de désérialisation à l'index %ld\n", index);
            break;
        }

        // Avancer l'index et incrémenter le compteur
        index += read;
        (*count)++;
    }

    // Ajuster la taille finale du tableau
    if (*count > 0)
    {
        message **tmp = realloc(msgs, (*count) * sizeof(message *));
        if (tmp)
            msgs = tmp;
    }

    return msgs;
}

void initialize_message(message *m)
{
    if (!m)
        return;
    m->code = 0;
    m->id = 0;
    m->lmess = 0;
    m->lsig = 0;
    m->mess[0] = '\0';
    m->sig[0] = '\0';
    m->nb = 0;
    m->numv = 0;
    m->prix = 0;
    memset(m->ip, 0, sizeof(m->ip));
    m->port = 0;
    m->cle[0] = '\0';
}

void free_message(message *m)
{
    if (m)
        free(m);
}

void free_messages(message **msgs, int count)
{
    if (!msgs)
        return;

    for (int i = 0; i < count; i++)
    {
        free(msgs[i]);
    }
    free(msgs);
}

void print_message(message *msg)
{
    printf("+----------------------+-----------------------------------------+\n");
    printf("| Field                | Value                                   |\n");
    printf("+----------------------+-----------------------------------------+\n");

    if (msg->code != 0)
        printf("| code                 | %-39u |\n", msg->code);

    if (msg->id != 0)
        printf("| id                   | %-39u |\n", msg->id);

    if (msg->lmess != 0)
    {
        printf("| lmess                | %-39u |\n", msg->lmess);
        char mess_display[MESS_SIZE + 1];
        strncpy(mess_display, msg->mess, msg->lmess);
        mess_display[msg->lmess] = '\0';
        printf("| mess                 | %-39.39s |\n", mess_display);
    }

    if (msg->lsig != 0)
    {
        printf("| lsig                 | %-39u |\n", msg->lsig);
        char sig_display[SIG_SIZE + 1];
        strncpy(sig_display, msg->sig, msg->lsig);
        sig_display[msg->lsig] = '\0';
        printf("| sig                  | %-39.39s |\n", sig_display);
    }

    if (msg->nb != 0)
        printf("| nb                   | %-39u |\n", msg->nb);

    if (msg->numv != 0)
        printf("| numv                 | %-39u |\n", msg->numv);

    if (msg->prix != 0)
        printf("| prix                 | %-39u |\n", msg->prix);

    int ip_is_zero = 1;
    for (int i = 0; i < 16; i++)
    {
        if (msg->ip[i] != 0)
        {
            ip_is_zero = 0;
            break;
        }
    }

    if (!ip_is_zero)
    {
        char ip_str[50]; // Un peu plus d'espace pour être sûr
        snprintf(ip_str, sizeof(ip_str), "%x:%x:%x:%x:%x:%x:%x:%x",
                 (msg->ip[0] << 8) | msg->ip[1],
                 (msg->ip[2] << 8) | msg->ip[3],
                 (msg->ip[4] << 8) | msg->ip[5],
                 (msg->ip[6] << 8) | msg->ip[7],
                 (msg->ip[8] << 8) | msg->ip[9],
                 (msg->ip[10] << 8) | msg->ip[11],
                 (msg->ip[12] << 8) | msg->ip[13],
                 (msg->ip[14] << 8) | msg->ip[15]);
        printf("| ip                   | %-39s |\n", ip_str);
    }

    if (msg->port != 0)
        printf("| port                 | %-39u |\n", msg->port);

    if (msg->cle[0] != '\0')
        printf("| cle                  | %-39.39s |\n", msg->cle);

    printf("+----------------------+-----------------------------------------+\n");
    printf("Taille de la cle : %ld\n", strlen(msg->cle));
}