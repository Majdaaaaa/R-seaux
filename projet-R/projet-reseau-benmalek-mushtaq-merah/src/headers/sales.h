#ifndef SALES_H
#define SALES_H

#include <stdint.h>
#include <../structs/sale.h>

/**
 * @brief Initialise une vente.
 * @param v Pointeur vers la structure sale à initialiser.
 * @param num_sale Numéro unique de la vente.
 * @param prix_depart Prix de départ de la vente.
 * @param superviseur_id Identifiant du superviseur de la vente.
 */
void sale_init(sale *v, uint32_t num_sale, uint32_t prix_depart, uint16_t superviseur_id);

/**
 * @brief Met à jour le prix courant et l'enchérisseur en tête d'une vente.
 * @param v Pointeur vers la structure sale à mettre à jour.
 * @param nouveau_prix Nouveau prix proposé.
 * @param nouvel_encherisseur Identifiant du nouvel enchérisseur en tête.
 */
void sale_update(sale *v, uint32_t nouveau_prix, uint16_t nouvel_encherisseur);

/**
 * @brief Termine une vente (la rend inactive).
 * @param v Pointeur vers la structure sale à clôturer.
 */
void sale_close(sale *v);

/**
 * @brief Recherche une vente par son numéro.
 * @param numv Numéro de la vente recherchée.
 * @param ss Tableau des ventes.
 * @param cmpt_sale Nombre de ventes dans le tableau.
 * @return L'indice de la vente trouvée, ou -1 si absente.
 */
int find_sale(uint32_t numv, sale *ss, int cmpt_sale);

/**
 * @brief Vérifie si une vente est active.
 * @param v Structure sale à vérifier.
 * @return 1 si la vente est active, 0 sinon.
 */
int sale_is_active(sale v);

/**
 * @brief Affiche les informations d'une vente (pour le debug).
 * @param v Pointeur vers la structure sale à afficher.
 */
void sale_print(sale *v);

/**
 * @brief Recherche si un pair est superviseur d'une vente.
 * @param id Identifiant du pair.
 * @param sales_list Tableau des ventes.
 * @param cmpt_sale Nombre de ventes.
 * @return Pointeur vers la vente supervisée, ou NULL si aucune.
 */
sale *is_superv(int id, sale *sales_list, int cmpt_sale);

#endif