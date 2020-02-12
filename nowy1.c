#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>


void handler_term(int a) {

	exit(1);

}

void handler_int(int a) {

	exit(2);

}

int main() {
	
	struct sigaction sact_new;
	struct sigaction sact_old;
	sact_new.sa_handler = handler_term;

	sigaction(SIGTERM, &sact_new, &sact_old);
	sact_new.sa_handler = handler_int;

	sigaction(SIGINT, &sact_new, &sact_old);

	printf("Proces 1\n");
	fflush(stdout);
	sleep(20);

	return 0;

}