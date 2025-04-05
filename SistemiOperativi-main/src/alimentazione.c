#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
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
int semid;
int kill_id;
int shmid;

int n_nuovi_atomi;
int step_alimentazione;

sharedInfo *shm_ptr;

void createIPCObjects();
void creaAtomo();
void sigterm_handler();
void sigchld_handler();
void sigalrm_handler();

int main(int argc, char ** argv) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <n_nuovi_atomi> <step_alimentazione>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    srand(getpid());

    n_nuovi_atomi = atoi(argv[2]);
    step_alimentazione = atoi(argv[3]);
    
    set_handler(SIGTERM, &sigterm_handler);
    set_handler(SIGCHLD, &sigchld_handler);
    set_handler(SIGALRM, &sigalrm_handler);
    createIPCObjects();



    wait_semaphore(semid, START_SEM);

    // Imposta il timer per generare un segnale SIGALRM ogni STEP secondi
    struct itimerval timer;
    timer.it_value.tv_sec = step_alimentazione;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = step_alimentazione;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("Errore nell'impostazione del timer");
        exit(EXIT_FAILURE);
    }

    while (1) {
        pause();
    }

    return 0;
}

void createIPCObjects() {
    // Creazione/Prelevo semaforo mutex
    semid = semget(SEM_KEY, 2, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("[Alimentazione]: Errore nella creazione del semaforo mutex");
        exit(EXIT_FAILURE);
    }

    // Crea la coda di messaggi per la terminazione dei processi
    if ((kill_id = msgget(KILL_KEY, IPC_CREAT | 0666)) == -1) {
        perror("[Alimentazione]: Errore nella creazione della coda di messaggi");
        exit(EXIT_FAILURE);
    }
    // Invio il pid al master
    struct queue k;
    k.type = 1;
    snprintf(k.msg, MSGSZ, "%d", getpid());
    if (msgsnd(kill_id, &k, (int)sizeof(k), 0) == -1) {
        perror("[Alimentazione]: Errore nell'invio del messaggio");
        exit(EXIT_FAILURE);
    }

    // Creazione/Prelievo shared memory
    shmid = shmget(SHM_KEY, sizeof(sharedInfo*), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("[Alimentazione]: Errore nella creazione della memoria condivisa");
        exit(EXIT_FAILURE);
    }
    // Assegno il puntatore alla memoria condivisa
    shm_ptr = (sharedInfo *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("[Alimentazione]: Errore nell'attaccamento alla memoria condivisa");
        exit(EXIT_FAILURE);
    }

}

void creaAtomo() {
    for (int i = 0; i < n_nuovi_atomi; i++) {
        // printf("creo atomo\n");
        pid_t pid;
        pid = fork();
        char numAtom[20];
        sprintf(numAtom, "%d", random_range(1, MAX_N_ATOMICO));
        if (pid == 0) {
            char *atomo_args[] = {"bin/atomo", "atomo", numAtom, "1", NULL};
            execve(atomo_args[0], atomo_args, NULL);
            perror("Errore nella creazione del processo atomo");
            exit(EXIT_FAILURE);
        }
        else if (pid == -1) {
            kill(shm_ptr->master_pid, SIGUSR1);
            exit(0);
        }
    }   
}

void sigterm_handler() {
    fprintf(stdout, "[Alimentazione] : Ricevuto SIGTERM, termino\n");
    exit(0);
}

void sigchld_handler() {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // printf("Processo figlio %d terminato\n", pid);
    }
    
}


void sigalrm_handler() {
    creaAtomo();
}
