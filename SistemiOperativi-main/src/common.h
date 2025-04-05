#ifndef common_h
#define common_h

#define START_SEM 1
#define MUTEX_SEM 0

#define MSG_KEY 92923

#define KILL_KEY 12345

#define SEM_KEY 49903

#define SHM_KEY 12344

#define MIN_N_ATOMICO 5
#define MAX_N_ATOMICO 118

#define MSGSZ 10

struct queue {
	long type;
	char msg[MSGSZ];
};

typedef struct _sharedInfo {
	int attivazioni;
	int scissioni;
	int enegia_prodotta;
	int energia_consumata;
	int scorie;
	int master_pid;
} sharedInfo;


// Funzioni per la gestione dei semafori
void wait_semaphore(int sem_id, int sem_num);
void signal_semaphore(int sem_id, int sem_num);
void destroy_semaphores(int sem_id);
int max(int a, int b);
int random_range(int min, int max);
struct sigaction set_handler(int sig, void (*func)(int));



#endif
