#ifndef SALE_H
#define SALE_H

#include <stdint.h>
#include <time.h>

typedef struct {
    uint32_t num_vente;
    uint32_t prix_depart;
    uint32_t prix_courant;
    uint16_t superviseur_id;
    uint16_t encherisseur_en_tete;
    int active; // 1 = en cours, 0 = terminée
} sale;

#endif