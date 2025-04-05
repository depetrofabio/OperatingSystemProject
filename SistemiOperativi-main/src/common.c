#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>

void wait_semaphore(int sem_id, int sem_num) {
    struct sembuf sem_op;
    sem_op.sem_num = sem_num;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;

    if (semop(sem_id, &sem_op, 1) == -1) {
        // perror("Errore nell'esecuzione dell'operazione wait sul semaforo");
        exit(EXIT_FAILURE);
    }
}

void signal_semaphore(int sem_id, int sem_num) {
    struct sembuf sem_op;
    sem_op.sem_num = sem_num;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;

    if (semop(sem_id, &sem_op, 1) == -1) {
        // perror("Errore nell'esecuzione dell'operazione signal sul semaforo");
        exit(EXIT_FAILURE);
    }
}

void destroy_semaphores(int sem_id) {
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("Errore nella rimozione dei semafori");
        exit(EXIT_FAILURE);
    }
}

int max(int a, int b) {
    return a > b ? a : b;
}

int random_range(int min, int max) {
    return rand() % (max - min + 1) + min;
}

// Codice preso da esempio progetto del professore E.Bini
struct sigaction set_handler(int sig, void (*func)(int)) {
	struct sigaction sa, sa_old;
	sigset_t mask;
	sigemptyset(&mask);
	sa.sa_handler = func;
	sa.sa_mask = mask;
	sa.sa_flags = 0;
	sigaction(sig, &sa, &sa_old);
	return sa_old;
}