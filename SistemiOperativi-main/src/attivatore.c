#include <unistd.h>
#include <errno.h>
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
#include "common.h"


// Variabili globali
int msg_id;
int kill_id;
int semid;
int shmid;
sharedInfo *shm_ptr;	
int step_attivatore;
// Prototipi di funzione
void createIPCObjects();
void sigterm_handler();



int main (int argc, char *a[]) {
	if (argc != 3) {
		fprintf(stderr, "Uso: %s <step_attivatore>\n", a[0]);
		exit(EXIT_FAILURE);
	}
	srand(getpid());
	step_attivatore = atoi(a[2]);
	set_handler(SIGTERM, &sigterm_handler);
	createIPCObjects();

	wait_semaphore(semid, START_SEM);

	// printf("Dimensione massima della coda dei messaggi: %lu byte\n", buf.msg_qbytes);
	// printf("Numero di messaggi nella coda: %lu\n", buf.msg_qnum);

	

	while (1) {
		struct queue rec;
		msgrcv(msg_id, &rec, sizeof(rec), 1, 0);
		wait_semaphore(semid, MUTEX_SEM);
		shm_ptr->attivazioni++;
		signal_semaphore(semid, MUTEX_SEM);
		// printf("[Attivatore]: Messaggio ricevuto: %s\n", rec.msg);
		struct queue snd;
		snd.type = atoi(rec.msg);

		int r = rand() % 10;
		r < step_attivatore ? strcpy(snd.msg, "OK") : strcpy(snd.msg, "KO"); // 80% di probabilità di successo
		if (msgsnd(msg_id, &snd, sizeof(snd), 0) == -1) {
			perror("[Attivatore]: Errore nell'invio del messaggio");
			exit(EXIT_FAILURE);
		}
	}
}

void createIPCObjects() {
	if ((msg_id = msgget(MSG_KEY, IPC_CREAT | 0666)) == -1) {
		perror("Errore nella creazione della coda di messaggi");
		exit(EXIT_FAILURE);
	}

	// Crea la coda di messaggi per la terminazione dei processi
	if ((kill_id = msgget(KILL_KEY, IPC_CREAT | 0666)) == -1) {
		perror("Errore nella creazione della coda di messaggi");
		exit(EXIT_FAILURE);
	}
	// Invio il pid al master
    struct queue k;
    k.type = 1;
    snprintf(k.msg, MSGSZ, "%d", getpid());
    if (msgsnd(kill_id, &k, (int)sizeof(k), 0) == -1) {
        perror("[Attivatore]: Errore nell'invio del messaggio");
        exit(EXIT_FAILURE);
    }



	if ((semid = semget(SEM_KEY, 2, IPC_CREAT | 0666)) == -1) {
		perror("Errore nella creazione del semaforo mutex");
		exit(EXIT_FAILURE);
	}

	if ((shmid = shmget(SHM_KEY, sizeof(sharedInfo*), IPC_CREAT | 0666)) == -1) {
		perror("Errore nella creazione della memoria condivisa");
		exit(EXIT_FAILURE);
	}
	// Assegno il puntatore alla memoria condivisa
	shm_ptr = (sharedInfo *)shmat(shmid, NULL, 0);
	if (shm_ptr == (void *)-1) {
		perror("Errore nell'attaccamento alla memoria condivisa");
		exit(EXIT_FAILURE);
	}

}
void sigterm_handler() {
	fprintf(stdout,"[Attivatore]: SIGTERM ricevuto, termino\n");
	exit(EXIT_SUCCESS);
}
