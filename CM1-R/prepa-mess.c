/*
                                1
  0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|    TYPE   |R |C |D |F |      INITIALE         |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|      LG               |       DATA...         |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

Ici : TYPE = 3, R = 0, C = 1, D= 0, F= 1 et le message est "Salut"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void){
	char *buf, initiale = 'Z';
	uint8_t lg = 6, type_opts; //LG

	/* initialisation du 1er octet */
	type_opts = 3 | ( 1 << 5 ) | ( 1 << 7 );

	/* allocation et remplissage de 0 du buffer */
	buf = malloc((3+lg) *  sizeof(char));
	memset(buf, 0, 3+lg);

	/* copie des champs du message dans le buffer */
	memcpy(buf, &type_opts, 1);
	memcpy(buf+1, &initiale, 1);
	memcpy(buf+2, &lg, 1);
	memcpy(buf+3, "Salut\n", lg);

	if(write(1, buf, lg+3) < lg+3){
		perror("write");
		exit(1);
	}

	/* libération de la mémoire réservée pour buf */
	free(buf);
	
	return 0;
}
