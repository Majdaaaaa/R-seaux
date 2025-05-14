#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX 1024    
/*
Dans cet exercice vous allez écrire un client IPv6 polymorphe qui essaie de se
connecter à dict. On ne sait pas à l’avance l’adresse de la machine, 
par conséquent, on ne sait pas si le serveur est en IPv4 ou en IPv6.

1. Étant donné que l’on connaît le nom de la machine à connecter, utilisez 
getaddrinfo pour récupérer son adresse. Faites attention : nous voulons que le 
client soit en IPv6, donc il faut choisir une bonne valeur 
pour hints.ai_family, et qu’il soit polymorphe, donc il faut choisir une bonne 
valeur pour hints.ai_flags
*/

int main(int argc,char **argv){
    if (argc != 2){
        perror("pas assez d'arguments");
        exit(EXIT_FAILURE);
    }
    char *mot = argv[1];
    char *machine = "localhost";
    struct addrinfo hints,*r,*p;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC; // famille de la socket sera quoi on sait pas encore si on sera en IPV6 ou IPV4
    hints.ai_flags = AI_V4MAPPED | AI_ALL;
    hints.ai_socktype = SOCK_STREAM;
    if ((getaddrinfo(machine,"2628",&hints,&r)) != 0){
        exit(EXIT_FAILURE);
    } // Q1
    char buf[MAX];
    p = r;
    int sock;
    while (p !=  NULL){
        if ( (sock = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) > 0){
            if (connect(sock,p->ai_addr,p->ai_addrlen) == 0){
                break;
            }
        }
        p = p->ai_next;
    }
    if (p == NULL) {
        printf("connexion pas abouttie\n");
        exit(EXIT_FAILURE);
    }
    else{
        printf("Connexion réussie\n");
        int r = recv(sock,buf,MAX,0);
        if (r < 0){
            perror("erreur dans le recv");
            exit(EXIT_FAILURE);
        }
        else{
            if (strncmp(buf,"220 ",4) == 0){
                printf("On a bien reçu %d\n",220);
                char buf_send[MAX];
                snprintf(buf_send,MAX,"DEFINE * %s",mot);
                printf("Le mot qu'on va envoyé : %s\n",mot);
                int s = send(sock,buf_send,strlen(buf_send),0);
                if (s < 0){
                    perror("erreur dans le send");
                    exit(EXIT_FAILURE);
                }
                else{
                    printf("message envoyé avec succes\n");
                    // exit(EXIT_SUCCESS);
                }
                char rep[MAX];
                memset(rep,0,MAX);
                int recu = 0;
                while(1){
                    int r = recv(sock,rep+recu,MAX-recu,0);
                    if (r< 0){
                        perror("problème dans le recv");
                        exit(EXIT_FAILURE);
                    }else{
                        recu+=r;
                    }
                    rep[recu]='\0';
                    if (recu == MAX){
                        printf("Le message du serveur %s\n",rep);
                        memset(rep,0,MAX);
                        recu = 0;
                    }
                    if (strstr(rep,"\r\n.\r\n") != NULL){
                        break;
                    }
                }
                exit(EXIT_SUCCESS);
            }
            else {
                printf("On a pas reçu %d\n",220);
                exit(EXIT_FAILURE);
            }

        }
    }//Q2

}