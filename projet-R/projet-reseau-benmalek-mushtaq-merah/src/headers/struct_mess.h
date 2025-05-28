#ifndef STRUCT_MESS_H
#define STRUCT_MESS_H

#include "peerList.h"
#include "structs/message.h"
/**
 * @brief  un message en buffer (sérialisation)
 * @param m strcuture à sérialiser
 * @param buf buffer à remplir
 * @return taille de du buffer (données)
 */
int struct2buf(message *m, char *buf);

/**
 * @brief Fonction de débogage qui permet de faire du pretty printing pour les structures
 * @param m Pointeur ver la structure message à afficher
 */
void print_message(message *m);

/**
 * @brief Initialise une strcuture messages avec des valeurs par défaut
 * @param m Pointeur vers une structure message à initialiser
 */
void initialize_message(message *m);

/**
 * @brief Transfome un buffer en un message (désérialisation)
 * @param buffer Buffer contenant les données sérialisées
 * @param len Pointeur vers un entier pour stocker la taille de la structure construite
 * @return Le message construit avec les données désérialisés
 */
message *buf2struct(char *buffer, int *len);

/**
 * @brief Transforme un tableau de structures message en un buffer pour envoi UDP
 * @param msgs Tableau de structures message à sérialiser
 * @param count Nombre de messages dans le tableau
 * @param buffer Buffer de sortie qui contiendra les données sérialisées
 * @return La taille totale des données écrites dans le buffer
 */
int structs2buf(message **msgs, int count, char *buffer);

/**
 * Transforme un buffer en un tableau de structures message
 * @param buffer Buffer contenant les données sérialisées
 * @param count Pointeur vers un entier qui recevra le nombre de messages dans le buffer
 * @param lenbuffer taille du buffer
 * @return Tableau de structures message (alloué avec un malloc)
 */
message **buf2structs(char *buffer, size_t *count, size_t lenbuffer);

void free_messages(message **msgs, int count);
void free_message(message *m);


#endif
