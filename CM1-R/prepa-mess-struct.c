/*
                                1
  0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|    TYPE   |R |C |D |F |      INITIALE         |                       |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|      LG               |       DATA...
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

Ici : TYPE = 3, R = 0, C = 1, D= 0, F= 1 et le message est "Salut"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct mess {
	uint8_t type_opts;
	char initiale;
	uint8_t lg;
	char *data;
} mess;

int main(void){
	mess m;
	char * buf;

	/* structure remplie de 0 */
	memset(&m, 0, sizeof(mess));

	/* initialisation des champs de la structure */
	m.type_opts = 3 | ( 1 << 5 ) | ( 1 << 7 );
        // m.type_opts = 0xa3;
	m.initiale = 'Z';
	m.lg = 5;
     	m.data = malloc((m.lg + 1) * sizeof(char));
	snprintf(m.data, 6, "Salut");

	/* allocation et remplissage de 0 du buffer */
	buf = malloc((3 + m.lg) *  sizeof(char));
	memset(buf, 0, 3 + m.lg);

	/* copie des champs de la structure dans le buffer */
	memcpy(buf, &(m.type_opts), 1);
	memcpy(buf+1, &m.initiale, 1);
	memcpy(buf+2, &m.lg, 1);
	memcpy(buf+3, m.data, m.lg);

	if(write(1, buf, m.lg + 3) < m.lg + 3){
		perror("write");
		exit(1);
	}

	/* libération de la mémoire réservée pour buf et m.data */
	free(m.data);
	free(buf);
	
	return 0;
}
