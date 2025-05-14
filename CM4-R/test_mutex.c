#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

int var = 0;
pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER;

void *serve(void *arg) {
	sleep(1);
  
	if(pthread_mutex_lock(&verrou)) {
		perror("pthread_mutex_lock");
		exit(1);
	}
  
	// modifier
	var += 1;
	// et faire une action avec cette nouvelle valeur
	printf("var = %d\n", var);

	if(pthread_mutex_unlock(&verrou)) {
		perror("pthread_mutex_unlock");
		exit(1);
	}
  
	return NULL;
}

int main(int argc, char *argv[]){
	int compt = 0;
	int len = atoi(argv[1]);
	pthread_t *tpthread = malloc(len*sizeof(pthread_t));

	while(compt < len){
      
		if (pthread_create(&tpthread[compt], NULL, serve, NULL)){
			perror("pthread_create");
			continue;
		}  
		compt++;
	}

	for(int i=0; i<len; i++)
		pthread_join(tpthread[i], NULL);

	free(tpthread);
	pthread_mutex_destroy(&verrou);
	return 0;
}
