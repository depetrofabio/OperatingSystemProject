#include "common.h"

#include <unistd.h>
#include <fcntl.h> // Add this line to include the missing header file
#include <sys/stat.h> // Include the missing header file

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

#define DEFAULT_N_ATOMI_INIT 10
#define DEFAULT_N_NUOVI_ATOMI 5
#define TIME_STAMP 1                        // intervallo di tempo tra due stampe

#define DEFAULT_ENERGY_DEMAND 0                  // quantità di energia richiesta dal master
#define DEFAULT_SIM_DURATION 60                     // durata della simulazione
#define DEFAULT_STEP_ALIMENTAZIONE 1
#define DEFAULT_STEP_ATTIVATORE 8
#define DEFAULT_ENERGY_EXPLODE_THRESHOLD 100000000     // soglia di energia per l'esplosione (KABOOOM)

#define DEFAULT 0
#define TIMEOUT 1
#define BLACKOUT 2
#define MELTDOWN 3
#define EXPLOSION 4


// Variabili globali
int shmid;
sharedInfo *shm_ptr;

int msg_id;
int kill_id; // id per la terminazione dei processi


int semid;

int energy_demand;
int sim_duration;
int step_alimentazione;
int n_atomi_init;
int n_nuovi_atomi; 
int step_attivatore;
int energy_explode_threshold;

int attivazioni_last, scissioni_last, energia_prodotta_last, energia_consumata_last, scorie_last;

// prototipi di funzione
void createIPCOBjects();
void creaAtomo();
void creaAttivatore();
void creaAlimentazione();
void terminate();
void printstats();
void signal_handler(int signum);
void sigchld_handler(int signum);
void sigalrm_handler(int sig);
void readSettingsFromUser();

int main () {
    readSettingsFromUser();
    set_handler(SIGUSR1, &signal_handler);
    set_handler(SIGCHLD, &sigchld_handler);
    set_handler(SIGINT, &signal_handler);
    srand(time(NULL));

    time_t current_time, start_time, init_time;
    time(&start_time);

    createIPCOBjects();
    creaAttivatore(); 
    creaAlimentazione();
    creaAtomo();

    semctl(semid, START_SEM, SETVAL, n_atomi_init + 2);


    int first = 1;
    time(&init_time);
    while(time(NULL) - init_time < sim_duration) {
        time(&current_time);
        if (current_time - start_time >= TIME_STAMP) {
            char * time_str = ctime(&current_time);
            time_str[strlen(time_str)-1] = '\0';

            wait_semaphore(semid, 0);
            start_time = current_time;
            printf("[Master]: --------------- %s ---------------\n",time_str);
            printstats();
            printf("[Master]: --------------------------------------------------------\n");
            printf("\n\n");
            if (first == -1) {
                shm_ptr->energia_consumata += energy_demand;
            }

            if (shm_ptr->energia_consumata > shm_ptr->enegia_prodotta && first == -1) {
                
                terminate(BLACKOUT);    
            } else if (shm_ptr->enegia_prodotta - shm_ptr->energia_consumata >= energy_explode_threshold) {
                terminate(EXPLOSION);
            }

            attivazioni_last = shm_ptr->attivazioni;
            scissioni_last = shm_ptr->scissioni;
            energia_prodotta_last = shm_ptr->enegia_prodotta;
            energia_consumata_last += energy_demand;
            scorie_last = shm_ptr->scorie;

            if (first == 1) {
                first = -1;
            }


            signal_semaphore(semid, MUTEX_SEM);
            // shm_ptr->energia_consumata += ENERGY_DEMAND;
        }

    }

    
    // ------------------ Fine Simulazione ------------------
    terminate(TIMEOUT);
    
}

void createIPCOBjects() {
    // Creazione coda di messaggi
    msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msg_id == -1) {
        perror("Errore nella creazione della coda di messaggi");
        exit(EXIT_FAILURE);
    }

    kill_id = msgget(KILL_KEY, IPC_CREAT | 0666);
    if (kill_id == -1) {
        perror("Errore nella creazione della coda di messaggi");
        exit(EXIT_FAILURE);
    }

    // Creazione shared memory
    shmid = shmget(SHM_KEY, sizeof(sharedInfo*), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Errore nella creazione della memoria condivisa");
        exit(EXIT_FAILURE);
    }
    // Assegno il puntatore alla memoria condivisa
    shm_ptr = (sharedInfo *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("Errore nell'attaccamento alla memoria condivisa");
        exit(EXIT_FAILURE);
    }
    // Inizializzo tutti i valori a 0
    shm_ptr->attivazioni = 0;
    shm_ptr->scissioni = 0;
    shm_ptr->enegia_prodotta = 0;
    shm_ptr->energia_consumata = 0;
    shm_ptr->scorie = 0;
    shm_ptr->master_pid = getpid();

    // Creazione semafori
    semid = semget(SEM_KEY, 2, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("Errore nella creazione del semaforo mutex");
        exit(EXIT_FAILURE);
    }
    if (semctl(semid, MUTEX_SEM, SETVAL, 1) == -1) {
        perror("Errore nell'inizializzazione del semaforo mutex");
        exit(EXIT_FAILURE);
    }
    if (semctl(semid, START_SEM, SETVAL, 0) == -1) {
        perror("Errore nell'inizializzazione del semaforo starting");
        exit(EXIT_FAILURE);
    }
}

void terminate(int condition) {
    switch (condition) {
        case BLACKOUT:
            printf("[Master]: BLACKOUT\n");
            break;
        case TIMEOUT:
            printf("[Master]: TIMEOUT\n");
            break;
        case MELTDOWN:
            printf("[Master]: MELTDOWN\n");
            break;
        case EXPLOSION:
            printf("[Master]: EXPLOSION\n");
            break;
        default:
            printf("[Master]: DEFAULT\n");
            break;
    }

    // Chiusura coda di messaggi
    // Ricevo tutti i messaggi sulla coda kill_id
    struct queue k;
    
    while (msgrcv(kill_id, &k, sizeof(k), 0, IPC_NOWAIT) != -1) {
        int kp = atoi(k.msg);
        kill(kp, SIGTERM);
    }

    wait_semaphore(semid, MUTEX_SEM);
    msgctl(msg_id, IPC_RMID, NULL);
    msgctl(kill_id, IPC_RMID, NULL);  
    signal_semaphore(semid, MUTEX_SEM);  
    printf("[Master]: Chiusura coda di messaggi\n");


    // Chiusura memoria condivisa
    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, NULL);
    printf("[Master]: Chiusura memoria condivisa\n");

    // Chiusura semafori
    destroy_semaphores(semid);
    exit(EXIT_SUCCESS);
}

void creaAtomo() {
    for (int i = 0; i < n_atomi_init; i++) {
        pid_t pid;

        char numAtom[20];
        sprintf(numAtom, "%d", random_range(1, MAX_N_ATOMICO));

        pid = fork();
        if (pid == 0) {
            char *atomo_args[] = {"bin/atomo", "atomo", numAtom, "0", NULL};
            execve(atomo_args[0], atomo_args, NULL);
            perror("[Master]: Errore nella creazione del processo atomo");
            exit(EXIT_FAILURE);
        }
        else if (pid == -1) {
            terminate(MELTDOWN);
        } else {
            // wait(NULL);
        }
    }   
}

void creaAlimentazione() {
    pid_t pid;

    char n_nuovi_atomi_str[20];
    sprintf(n_nuovi_atomi_str, "%d", n_nuovi_atomi);
    char step_alimentazione_str[20];
    sprintf(step_alimentazione_str, "%d", step_alimentazione);

    pid = fork();


    if (pid == 0) {
        char *alimentatore_args[] = {"bin/alimentazione", "alimentazione", n_nuovi_atomi_str, step_alimentazione_str, NULL};
        execve(alimentatore_args[0], alimentatore_args, NULL);
        perror("[Master]: Errore nella creazione del processo alimentazione");
        exit(EXIT_FAILURE);
    }
    else if (pid == -1) {
        terminate(MELTDOWN);
    }
}

void creaAttivatore() {
    pid_t pid;

    char step_attivatore_str[20];
    sprintf(step_attivatore_str, "%d", step_attivatore);


    pid = fork();
    if (pid == 0) {
        char *attivatore_args[] = {"bin/attivatore", "attivatore", step_attivatore_str, NULL};
        execve(attivatore_args[0], attivatore_args, NULL);
        perror("[Master]: Errore nella creazione del processo attivatore");
        exit(EXIT_FAILURE);
    }
    else if (pid == -1) {
        terminate(MELTDOWN);
    }
}

void printstats() {
    printf("[Master]: Attivazioni: %d\n", shm_ptr->attivazioni);
    printf("[Master]: Attivazioni dell'ultimo secondo: %d\n", shm_ptr->attivazioni - attivazioni_last);
    printf("\n");
    printf("[Master]: Scissioni: %d\n", shm_ptr->scissioni);
    printf("[Master]: Scissioni dell'ultimo secondo: %d\n", shm_ptr->scissioni - scissioni_last);
    printf("\n");
    printf("[Master]: Energia prodotta: %d\n", shm_ptr->enegia_prodotta);
    printf("[Master]: Energia prodotta nell'ultimo secondo: %d\n", shm_ptr->enegia_prodotta - energia_prodotta_last);
    printf("\n");
    printf("[Master]: Energia consumata: %d\n", shm_ptr->energia_consumata);
    printf("[Master]: Energia consumata nell'ultimo secondo: %d\n", energy_demand);
    printf("\n");
    printf("[Master]: Scorie: %d\n", shm_ptr->scorie);
    printf("[Master]: Scorie dell'ultimo secondo: %d\n", shm_ptr->scorie - scorie_last);   
}

void signal_handler(int signum) {
    switch(signum) {
        case SIGINT:
            terminate(DEFAULT);
            break;
        case SIGUSR1:
            terminate(MELTDOWN);
            break;
        default:
            break;
    }
}

void sigchld_handler(int signo) {
    (void)signo;

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
    }
}

int readIntegerInputWithDefault(const char *prompt, int defaultValue) {
    char input_buffer[256]; // Scegli una dimensione appropriata
    int value;

    printf("%s [default=%d]: ", prompt, defaultValue);
    fgets(input_buffer, sizeof(input_buffer), stdin);
    
    if (input_buffer[0] != '\n') {
        sscanf(input_buffer, "%d", &value);
        return value;
    } else {
        return defaultValue;
    }
}

void readSettingsFromUser() {
    printf("Inserire i valori richiesti di seguito, enter per usare il valore di default\n");
    sim_duration = readIntegerInputWithDefault("Inserire la costante SIM_DURATION per la durata temporale (in secondi) della simulazione", DEFAULT_SIM_DURATION);
    energy_demand = readIntegerInputWithDefault("Inserire la quantità di energia richiesta dal master 0 == !BLACKOUT", DEFAULT_ENERGY_DEMAND);
    energy_explode_threshold = readIntegerInputWithDefault("Inserire la soglia di energia per l'esplosione", DEFAULT_ENERGY_EXPLODE_THRESHOLD);
    n_atomi_init = readIntegerInputWithDefault("Inserire il numero di atomi iniziali", DEFAULT_N_ATOMI_INIT);
    n_nuovi_atomi = readIntegerInputWithDefault("Inserire il numero di atomi che l'alimentazione deve creare", DEFAULT_N_NUOVI_ATOMI);
    step_alimentazione = readIntegerInputWithDefault("Inserire l'intervallo di tempo per ogni nuova creazione dell'alimentazione", DEFAULT_STEP_ALIMENTAZIONE);
    step_attivatore = readIntegerInputWithDefault("Inserire il valore di STEP_ATTIVATORE, ovvero la probabilità con cui ordinerà scissioni agli atomi", DEFAULT_STEP_ATTIVATORE);
}
