#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/syslimits.h>
#include <string.h>
#include <unistd.h>

int main (int argc, char* argv[]){
    if(argc < 2){
        fprintf(stderr,"usage : ./a.out <adresse>\n");
        return EXIT_FAILURE;
    }
    uint16_t entete[6];
    char* adresse = argv[1];
    printf("%s\n",adresse); // debug
    
    char* buf; // le buffer sert peut-etre a rien , mess est deja une sorte de buffer
    srand(time(NULL));
    uint16_t id = rand() % 65535;
    entete[0] = htons(id);// le champs ID 2 octets (= 16 bits)
    entete[1] = htons(1 << 7); // Bit RD activé (2 octets)
    entete[2] = htons(1); // Champ QDCOUNT (2 octets)
    entete[3] = htons(0); // ANCOUNT (2 octets)
    entete[4] = htons(0); // NSCOUNT (2 octets)
    entete[5] = htons(0); // ARCOUNT (2 octets)
    
    
    
    
    // encodage de l'adresse
    // ici : www.wikepedia.fr
    char * token = strtok(adresse,".");
    char adresse_mess[ARG_MAX];
    int pos =0;
    while(token!=NULL){
        printf("token %s\n",token);
        int taille = strlen(token);
        printf("taille token %d \n",taille);
        adresse_mess[pos] = taille;
        pos++;
        for (int i = 0 ; token[i] != '\0' ; i++ ){
            adresse_mess[pos] = token[i];
            pos++;
        }
        token = strtok(NULL,".");
    }
    adresse_mess[pos]=0x00;
    pos++;
    
    uint16_t qtype = htons(1);
    uint16_t qclass = htons(1);
    
    
    buf = malloc(sizeof(entete)+pos + sizeof(qtype)+sizeof(qclass));
    
    memcpy(buf, entete, sizeof(entete));
    memcpy(buf+sizeof(entete),adresse_mess,pos);
    memcpy(buf+sizeof(entete)+pos,&qtype,sizeof(qtype));
    memcpy(buf+sizeof(entete)+pos+sizeof(qtype),&qclass,sizeof(qclass));
                 
    

    int fd = open("requete", O_RDWR | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        perror("Erreur d'ouverture du fichier");
        return EXIT_FAILURE;
    }
    
    ssize_t written = write(fd, buf, sizeof(entete)+pos + sizeof(qtype)+sizeof(qclass));
    if (written == -1) {
        perror("Erreur d'écriture");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Données écrites dans le fichier requete\n");
    
    close(fd);
    return EXIT_SUCCESS;
    
    
}
