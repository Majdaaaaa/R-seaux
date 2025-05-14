#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

int var = 0;
pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t vcond = PTHREAD_COND_INITIALIZER;

void *serve(void *arg) {
	if(pthread_mutex_lock(&verrou)) {
		perror("pthread_mutex_lock");
		exit(1);
	}
  
	if(var == 0)
        if(pthread_cond_wait(&vcond, &verrou)) {
			perror("pthread_mutex_lock");
			exit(1);
		}
		
	var += 1;
	printf("var = %d\n", var);
  
	if(pthread_mutex_unlock(&verrou)) {
		perror("pthread_mutex_unlock");
		exit(1);
	}
	printf("--var = %d\n", var);
  
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

	srand(getpid());
	var = rand()%100;

	printf("thread principal : var = %d\n", var);

	if(pthread_mutex_lock(&verrou)) {
		perror("pthread_mutex_lock");
		exit(1);
	}
  
	/* if(pthread_cond_broadcast(&vcond)) {
	       perror("pthread_cond_broadcast");
	       exit(1);
	   } 
	*/

	for(int i=0; i<len; i++) {
		if(pthread_cond_signal(&vcond))
			perror("pthread_cond_signal");
	}
    
	if(pthread_mutex_unlock(&verrou)) {
		perror("pthread_mutex_unlock");
		exit(1);
	}
  
	for(int i=0; i<len; i++) {
		if(pthread_join(tpthread[i], NULL)) 
			perror("pthread_mutex_unlock");
	}

	free(tpthread);
	pthread_mutex_destroy(&verrou);
	pthread_cond_destroy(&vcond);
	return 0;
}
