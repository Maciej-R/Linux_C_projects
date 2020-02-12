#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>

/*
Maciej Ryœnik
28.11.2019

Program creates 2 child prosesses
waits for end of their execution and
prints out theirs PID and temination signal number

Requires gcc -pthread
*/

/*
Semaphore is used to properly identify process (int num),
this allows to use execl with proper path

Synchronization description:

At 1) main process owns sem, creates process 1 jumps to 2)
2) main process releases sem, yields execution and waits for sem to be available again
   while loop ensures that execution won't continue even if 
   scheduler chooses main process to be executed before child
child_code: is exectued when child process gets sem 
   first one at 3) gets its code, releases sem and signals to main process that it has completed its task
4) main process continues execution, creates another child, increases num, signals to sem and 
   waits for child process to end
process child 2 from fork at 4) executes goto child_code and does its task
*/

inline void error_exit(const char*);
struct shared {
	int num;
	sem_t sem;
	bool first_ready;
};

int main(){

	//Shared memory
	void* shmem = mmap(NULL, sizeof(shared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	shared* shs = (shared*)shmem;
	sem_init(&shs->sem, 2, 1);
	shs->num = 0;
	//1)
	sem_wait(&shs->sem);

	pid_t fret = fork();
	if(fret != 0) ++shs->num;
	//num == 1
	
child_code:
	
	if (fret == 0) {
				
		//Path to current directory
		char* cwd = (char*)malloc(PATH_MAX);
		//Returns argument pointer upon successd
		if (getcwd(cwd, PATH_MAX) != cwd) error_exit("getcwd");

		int len = strlen(cwd);
		sem_wait(&shs->sem);
		//First child	
		if (shs->num == 1) {
			
			//3)
			const char* pcode = "/nowy1.code";
			int ls = strlen(pcode);
			if (len + ls < PATH_MAX) {
			
				//Path to code - assuming names like those pointed by pcode and cwd
				memcpy(&cwd[len], pcode, ls);
				cwd[len + ls] = '\0';
				printf(cwd);
				//Signal done
				shs->first_ready = true;
				sem_post(&shs->sem);
				//Swap program code
				execl(cwd, NULL);
			
			}
			else {

				error_exit("path too long");			

			}

		}//Second child
		else {
			
			const char* pcode = "/nowy2.code";
			int ls = strlen(pcode);
			if (len + ls < PATH_MAX) {

				memcpy(&cwd[len], pcode, ls);
				cwd[len + ls] = '\0';
				sem_post(&shs->sem);
				execl(cwd, NULL);

			}
			else {

				error_exit("path too long");


			}

		}

	}//Second child process num == 1
	else if(shs->num < 2){

		//2)
		while (!shs->first_ready) {
			sem_post(&shs->sem);
			pthread_yield();
			sem_wait(&shs->sem);
		}
		//4)
		fret = fork();
		if (fret != 0) {
			++shs->num;
		}
		else {

			goto child_code;
		}
	}
	sem_post(&shs->sem);

	int e, state;
	//Waits for childern, print out info
	while ((e = wait(&state)) != -1) {

		printf("PID: %d, sig: %d\n", e, WEXITSTATUS(state));
		printf("%d\n", WIFEXITED(state));

	}

    //printf("hello from SO3!\n");
    return 0;
}

inline void error_exit(const char* m) {

	perror(m);
	exit(EXIT_FAILURE);

}