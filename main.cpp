#include <cstdio>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <limits.h>
#include <math.h>
#include <poll.h>

/*
Maciej Ryœnk
25.11.2019

Program creates consumer and producer
synchronized by semaphore
working in separate threads (required linking with -pthread)
*/


//Simple struct used in shared memory
struct shstruct {

	sem_t mutex;
	//bool used to check if other part performed its action
	bool consumed;
	int num;
	//stdio input read to it
	char str[128];

};

void* consument(void *);
//Consise error handling
inline void error_exit(const char*);

int main(){

	//Creating shared memory at kernel-assigned address (NULL param), rw, without mapping of file (MAP_ANONYMOUS)
	//alternative way but unnecessary at thread level
	//void* shmem = mmap(NULL, sizeof(shstruct), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	void* shmem = malloc(sizeof(shstruct));
	shstruct* shs = (shstruct*)shmem;
	sem_init(&(shs->mutex), 0, 1);
	shs->consumed = true;

	//New thred attributes
	pthread_attr_t attr;
	if (pthread_attr_init(&attr)) error_exit("attr_init");
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE)) error_exit("setdetachstat");
	//Creating thread
	pthread_t thread_id = 1;
	if (pthread_create(&thread_id, &attr, consument, shmem)) error_exit("pthread_create");
	//Clean
	if (pthread_attr_destroy(&attr)) error_exit("attr_destory");
	
	for (int prev_num = -1; ;) {

		if (prev_num == INT_MAX) break;
		sem_wait(&shs->mutex);
		//Critical section
		if (shs->consumed) {
		
			//Read data
			scanf("%128s", shs->str);
			//Mark data available
			shs->consumed = false;
			++(shs->num);

		}
		//
		sem_post(&shs->mutex);
		//Yield control to enable consument to work
		pthread_yield();

	}

	munmap(shmem, sizeof(shstruct));

    //printf("hello from SO2!\n");
    return 0;
}

//Function reads data from shared memory and prints it out
void* consument(void * shs) {

	shstruct* shared = (shstruct*)shs;

	for (;;) {

		sem_wait(&shared->mutex);
		//Critical section
		if (!shared->consumed) {
		
			printf("%d ; ", shared->num);
			printf("%s\n", shared->str);
			fflush(stdout);
			//Mark consumed
			shared->consumed = true;
		
		}
		//
		sem_post(&shared->mutex);
		pthread_yield();

	}

	return nullptr;
}

inline void error_exit(const char* msg) {

	perror(msg);
	exit(EXIT_FAILURE);

}