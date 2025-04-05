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
int numAtom;

int shmid;
sharedInfo *shm_ptr;

int msg_id;
int kill_id; // coda per la terminazione dei processi

int semid;

// Prototipi di funzione
void createIPCObjects();
void scissione();
void energy(int n, int c);
void sigterm_handler();
void sigchld_handler();

void createIPCObjects() {
    // Creazione/Prelevo coda di messaggi
    if ((msg_id = msgget(MSG_KEY, IPC_CREAT | 0666)) == -1) {
        perror("[Atomo]: Errore nella creazione della coda di messaggi");
        exit(EXIT_FAILURE);
    }
    // Creazione/Prelevo shared memory
    shmid = shmget(SHM_KEY, sizeof(sharedInfo*), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("[Atomo]: Errore nella creazione della memoria condivisa");
        exit(EXIT_FAILURE);
    }
    // Assegno il puntatore alla memoria condivisa
    shm_ptr = (sharedInfo *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("[Atomo]: Errore nell'attaccamento alla memoria condivisa");
        exit(EXIT_FAILURE);
    }

    // Creazione/Prelevo semaforo mutex
    semid = semget(SEM_KEY, 2, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("[Atomo]: Errore nella creazione del semaforo mutex");
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <atomo> <numAtom> <start>\n", argv[0]);
        exit(EXIT_FAILURE);
    }



    set_handler(SIGTERM, &sigterm_handler);
    set_handler(SIGCHLD, &sigchld_handler);
    srand(getpid());
    createIPCObjects();

    // Aspetta il via dal master

    // Preleva dagli argomenti il suo numero atomico
    numAtom = atoi(argv[2]);
    // printf("[Atomo]: Sono l'atomo %d con numAtom = %d\n", getpid(), numAtom);
    // printf("Numero atomico padre prescissione: %d\n", numAtom);
    if (atoi(argv[3]) == 0) {
        wait_semaphore(semid, START_SEM);
    }

    // Invia all'attivatore il suo pid
    struct queue snd;
    snd.type = 1;
    // Manda il suo pid all'attivatore come stringa
    snprintf(snd.msg, MSGSZ, "%d", getpid());
    // fprintf(stdout, "[Atomo]: Invio il mio pid all'attivatore %s\n", snd.msg);

    if (msgsnd(msg_id, &snd, (int)sizeof(snd), 0) == -1) {
        // perror("[Atomo]: Errore nell'invio del messaggio");
        exit(EXIT_FAILURE);
    }

    // sleep(2);

    struct queue rec;
    if (msgrcv(msg_id, &rec, sizeof(rec), getpid(), 0) == -1) {
        // perror("[Atomo]: Errore nella ricezione del messaggio");
        exit(EXIT_FAILURE);
    }
    // fprintf(stdout, "[Atomo]: Messaggio ricevuto: %s\n", rec.msg);
    if (strcmp(rec.msg, "OK") == 0) {
        if (numAtom > MIN_N_ATOMICO) {
            scissione();
            exit(EXIT_SUCCESS);
            
        } else {
            wait_semaphore(semid, MUTEX_SEM);
            shm_ptr->scorie++;
            signal_semaphore(semid, MUTEX_SEM);

            // fprintf(stdout, "[Atomo]: SCORIA\n");
            // fprintf(stdout, "\n");
            exit(EXIT_SUCCESS);
        }
    } else {
        // fprintf(stdout, "[Atomo]: NOPE\n");
        // fprintf(stdout, "\n");
        exit(EXIT_SUCCESS);
    }
}


void scissione() {
    // fprintf(stdout, "[Atomo]: SCISSIONE\n");
    wait_semaphore(semid, MUTEX_SEM);
    shm_ptr->scissioni++;
    signal_semaphore(semid, MUTEX_SEM);

    pid_t pid;
    pid = fork();

    int child_numAtom = random_range(1, numAtom-1);
    numAtom -= child_numAtom;

    // printf("Numero atomico padre postscissione: %d\n", numAtom);
    // printf("Numero atomico figlio: %d\n", child_numAtom);


    char numAtom_str[20];
    sprintf(numAtom_str, "%d", child_numAtom);
    if (pid == 0) {
        char *atomo_args[] = {"bin/atomo", "atomo", numAtom_str, "1", NULL};
        execve(atomo_args[0], atomo_args, NULL);
        perror("Errore nella creazione del processo atomo");
        exit(EXIT_FAILURE);
    }
    else if (pid == -1) {
        kill(shm_ptr->master_pid, SIGUSR1);
        exit(0);
    } else {
        wait_semaphore(semid, MUTEX_SEM);
        energy(numAtom, child_numAtom);
        signal_semaphore(semid, MUTEX_SEM);
    }
}

void energy (int n, int c) {
    int f = n > c ? n : c;
    int e = n*c - f;
    shm_ptr->enegia_prodotta += e;
}

void sigterm_handler() {
    exit(0);
}

void sigchld_handler() {
    pid_t child_pid;
    int status;

    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            // printf("Il processo figlio %d è terminato con stato di uscita: %d\n", child_pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Il processo figlio %d è stato terminato da un segnale: %d\n", child_pid, WTERMSIG(status));
        } else {
            printf("Il processo figlio %d è terminato in modo anomalo\n", child_pid);
        }
        exit(0);
    }
}