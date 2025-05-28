#ifndef MESSAGE
#define MESSAGE
#include <stdint.h>

#define MTU 1200 // < MTU
#define MESS_SIZE 255
#define SIG_SIZE 255
#define CLE_SIZE 60

typedef struct
{
    uint8_t code; // code du message
    uint16_t id;  // identifiant du pair

    uint8_t lmess;        // taille du message
    uint8_t lsig;         // taille de la signature
    char mess[MESS_SIZE]; // le message
    char sig[SIG_SIZE];   // la signature

    uint16_t nb;   // nombre de pair qui ont signé le message ou qui ont abandonné la vente (ça depends du message)
    uint32_t numv; // numéro de la vente
    uint32_t prix; // prix de la vente

    uint8_t ip[16];     // l'adresse IP (sur 16 octets) (psq on est en IPv6?)
    uint16_t port;      // numéro de port
    char cle[CLE_SIZE]; // clé
    // uint16_t ids_disparus[];
} message;

#endif