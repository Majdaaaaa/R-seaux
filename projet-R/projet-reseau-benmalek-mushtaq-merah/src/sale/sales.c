#include "../headers/sales.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../headers/utils.h"

void sale_init(sale *v, uint32_t num_sale, uint32_t prix_depart, uint16_t superviseur_id)
{
    v->num_vente = num_sale;
    v->prix_depart = prix_depart;
    v->prix_courant = prix_depart;
    v->superviseur_id = superviseur_id;
    v->encherisseur_en_tete = 0;
    v->active = 1;
}

void sale_update(sale *v, uint32_t nouveau_prix, uint16_t nouvel_encherisseur)
{
    v->prix_courant = nouveau_prix;
    v->encherisseur_en_tete = nouvel_encherisseur;
}

void sale_close(sale *v)
{
    v->active = 0;
}

int sale_is_active(sale v)
{
    return v.active;
}

int find_sale(uint32_t num_vente, sale *sales_list, int cmpt_sale)
{
    for (int i = 0; i < cmpt_sale; i++)
    {
        if (sales_list[i].num_vente == num_vente)
            return i;
    }
    return -1;
}

void sale_print(sale *v)
{
    printf("%s------------------- Vente #%u -------------------%s\n", COLOR_BLUE, v->num_vente, COLOR_RESET);
    printf("%sPrix de départ      :%s %s%u%s\n", COLOR_CYAN, COLOR_RESET, COLOR_GREEN, v->prix_depart, COLOR_RESET);
    printf("%sPrix courant        :%s %s%u%s\n", COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, v->prix_courant, COLOR_RESET);
    printf("%sSuperviseur         :%s %u\n", COLOR_CYAN, COLOR_RESET, v->superviseur_id);
    printf("%sEnchérisseur en tête:%s %u\n", COLOR_CYAN, COLOR_RESET, v->encherisseur_en_tete);
    printf("%sStatut              :%s %s%s%s\n", COLOR_CYAN, COLOR_RESET,
           v->active ? COLOR_GREEN : COLOR_RED,
           v->active ? "ACTIVE" : "TERMINÉE",
           COLOR_RESET);
    printf("%s--------------------------------------------------%s\n", COLOR_BLUE, COLOR_RESET);
}

sale *is_superv(int id, sale *sales_list, int cmpt_sale)
{
    for (int i = 0; i < cmpt_sale; i++)
    {
        if (id == sales_list[i].superviseur_id)
        {
            return &sales_list[i];
        }
    }
    return NULL;
}