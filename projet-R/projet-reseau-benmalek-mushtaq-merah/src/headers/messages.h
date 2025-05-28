#ifndef MESSAGES_H
#define MESSAGES_H

#include "struct_mess.h"
#include <stdint.h>

// ^ Processus de consensus
/**
 * @brief Message validé envoyé par un pair au superviseur.
 *
 * @note UDP (je sais pas si c'est multicast ou pas )
 *
 * @param id identifiant du pair
 * @param mess message du pair au superviseur
 * @param sig signature du pair
 *
 * @warning  malloc effectué ici donc faut free.
 *
 * @return pointeur vers la structure qui forme le message à envoyer
 */
message *processus_consensus_mess1(uint16_t id, char *mess, char *sig);

/**
 * @brief remplir une strcuture avec la signature de chaque pairs (à concaténer dans les message de processus_consensus_mess4)
 *
 * @note à envoyé dans cette état a personne.
 *
 * @param id d'un pair
 * @param sig d'un pair
 * @warning  malloc effectué ici donc faut free.
 *
 * @see processus_consensus_mess4() pour + d'infos de l'utilisation.
 */
message *signature_individuelle(uint16_t id, char *sig);

/**
 * @brief Message envoyé par le superviseur aux pairs.
 *
 * @note multidiffusion UDP (adresse enchere)
 *
 * @param nb nombre de pairs ayant signé le message
 * @param mess le message
 * @param sig signature du superviseur
 * @param id_sup id du superviseur
 *
 * @warning  malloc effectué ici donc faut free.
 *
 * @return tableau de pointeur structure (car on concatène plusieurs message (voir struct_mess pour des methodes de sérialisation et désérialisation de ces tableaux ))
 */
message **processus_consensus_mess4(uint16_t nb, char *mess, char *sig, uint16_t id_sup);

// ^ Rejoindre une enchère
/**
 * @brief Message envoyé par un pair  pour rejoindre une enchère
 *
 * @note multidiffusion
 * @warning  malloc effectué ici donc faut free.
 *
 * @return la structure qui forme le message
 */
message *rejoindre_enchere_mess1();

/**
 * @brief Message envoyé par les pairs au pair qui veut rejoindre l'enchère
 *
 * @note unicast TCP
 *
 * @param id identifiant du pair (qui réponds)
 * @param ip adressePerso du pair
 * @param port le numéro de port de cette adresse
 *
 * @warning  malloc effectué ici donc faut free.
 * @return la structure qui forme le message
 */
message *rejoindre_enchere_mess3(uint16_t id, uint8_t ip[16], uint16_t port);

/**
 * @brief Message envoyé par un pair qui veut rejoindre au superviseur pour rejoindre l'enchère
 *
 * @note unicast TCP (TLS)
 *
 * @param id identifiant du pair qui veut rejoindre
 * @param ip son adresse IP adressePerso
 * @param port le numero de port de l'adresse
 * @param  cle la cle de ce pair
 *
 * @warning  malloc effectué ici donc faut free.
 * @return  la structure qui forme le message
 */
message *rejoindre_enchere_mess4(uint16_t id, uint8_t ip[16], uint16_t port, char *cle);

/**
 * @brief Réponse du superviseur au message 4, si identifiant valide.
 *
 * @warning  malloc effectué ici donc faut free.
 * @return la structure qui forme le message
 */
message *identifiant_valide();

/**
 * @brief  Réponse du superviseur au message 4, si identifiant non valide.
 *
 * @param id identifiant choisi par le superviseur et attitré au pair qui viens de rejoindre
 *
 * @warning  malloc effectué ici donc faut free.
 * @return la structure qui forme le message
 */
message *identifiant_non_valide(uint16_t id);

/**
 * @brief fonction auxiliaire pour écrire le message INFO, car il reviens souvent
 *
 * @param id identifiant
 * @param ip adresse ip
 * @param port numéro de port
 * @param cle clé
 *
 * @warning  malloc effectué ici donc faut free.
 * @return structure représentant le message
 */
message *suffixe_info(uint16_t id, uint8_t ip[16], uint16_t port, char *cle);

/**
 * @brief Message envoyé à chaque pair par le superviseur, pour transmettre l'id, l'ip, le port et la clé du nouveau membre
 *
 * @note unicast TCP
 *
 * @param id identifiant du nouveau membre
 * @param ip adressePerso du nouveau membre
 * @param port port du nouveau membre
 * @param cle cle du nouveau membre
 *
 * @warning  malloc effectué ici donc faut free.
 * @return la structure formant le message à envoyé
 */
message *rejoindre_enchere_mess5(uint16_t id, uint8_t ip[16], uint16_t port, char *cle);

/**
 * @brief Message envoyé par le superviseur au pair qui veut rejoindre avec toute les informations sur chaque pair dy système
 *
 * @note unicast TCP
 *
 * @param peers la liste des pairs présents
 *
 * @warning  malloc effectué ici donc faut free.
 * @return tableau de structure avec tout les messages à envoyé
 */
message **rejoindre_enchere_mess6(PeerList *peers);

// ^ Déroulement d'une vente
/**
 * @brief Message envoyé par le pair qui veut initier une vente à tout les autres pairs.
 *
 * @note multidiffusion
 *
 * @param id identifiant du pair initiateur de la vente (superviseur de la vente)
 * @param numv numéro de la vente (concaténation de l'identifiant du pair et du nombre de vente qu'il a initier)
 * @param prix prix initial de la vente
 *
 * @warning  malloc effectué ici donc faut free.
 * @return la structure formant le message à envoyé
 */
message *initier_vente(uint16_t id, uint32_t numv, uint32_t prix);

/**
 * @brief Message envoyé par un pair pour enchérir sur une vente
 *
 * @note multidiffusion
 *
 * @param id identifiant du pair qui enchéris
 * @param numv numéro de la vente sur laquelle il veut enchérir
 * @param prix le nouveau prix qu'il propose
 *
 * @warning  malloc effectué ici donc faut free.
 * @return la structure formant le message à envoyé
 */
message *encherir_vente(uint16_t id, uint32_t numv, uint32_t prix);

/**
 * @brief Message envoyé par le superviseur de la vente à tout les autres pairs
 *
 * @param m pointeur vers un message (de encherur_vente)
 *
 * @note multidiffusion
 */
void encherir_superviseur(message *m);

// ^ finalisation d'une vente

/**
 * @brief Message envoyé par le superviseur (de la vente pas de l'enchere) à tout les pairs
 *
 * @note multidiffusion
 *
 * @param id identifiant du superviseur de la vente
 * @param numv numéro de la vente
 * @param prix dernier prix de la vente
 *
 * @warning  malloc effectué ici donc faut free.
 * @return la structure formant le message à envoyé
 */
message *finalisation_mess1(uint16_t id, uint32_t numv, uint32_t prix);

/**
 * @brief message envoyé par le superviseur de la vente à tout les pairs, pour annoncer le gagant de la vente
 *
 * @note multidiffusion
 *
 * @param id identifiant du gagant de la vente si y'en a un sinon l'id du superviseur de la vente
 * @param numv numéro de la vente
 * @param prix de la derniere enchère validée ou le prix initial si personne gagne
 *
 * @warning  malloc effectué ici donc faut free.
 * @return la structure formant le message à envoyé
 */
message *finalisation_mess2(uint16_t id, uint32_t numv, uint32_t prix);

// ^ quitter une enchere
/**
 * @brief Message envoyé par un pair pour annoncer qu'il quitter l'enchere
 *
 * @note multidiffusion UDP (adresse enchere)
 *
 * @param id son identifiant
 *
 * @warning malloc effectué ici donc faut free.
 * @return la structure formant le message à envoyé
 */
message *quitter_enchere(uint16_t id);

// ^ Message non valide

/**
 * @brief Message envoyé par le superviseur, pour refuser une enchère
 *
 * @note multidiffusion UDP
 *
 * @param code soit 14 si l'enchère est rejeter pour cause de concurrence, 15 sinon.
 * @param id identifiant du pair qui se fait refuser
 * @param numv de la vente rejeter
 * @param prix de la vente rejeter
 *
 * @warning malloc effectué ici donc faut free.
 * @return la structure formant le message à envoyé
 */
message *rejet_enchere(uint8_t code, uint16_t id, uint32_t numv, uint32_t prix);

//^ Gestion des pairs qui répondent plus

/**
 * @brief Message envoyé a tout les pairs à la disparition d'un superviseur
 *
 * @note multidiffusion UDP
 *
 * @param id identifiant du superviseur disparu
 * @param numv numero de la vente
 *
 * @warning malloc effectué ici donc faut free.
 * @return la structure formant le message à envoyé
 */
message *disparition_superviseur(uint16_t id, uint32_t numv);

/**
 * @brief Message envoyé par un pair constatnt des pairs disparus.
 *
 * @note multidiffusion UDP (enchere)
 *
 * @param id id du pair qui annonce la disparition
 * @param nb nombre de pairs disparus
 *
 * @warning malloc effectué ici donc faut free.
 * @return la structure formant le message à envoyé
 */
message **disparition_pair(uint16_t id, uint16_t nb,Peer *disparus,int *cmp);
 #endif