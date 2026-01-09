#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common/logger.h"

/**
 * @union semun
 * @brief Unione richiesta da semctl() per l'inizializzazione dei semafori.
 *
 * Definita esplicitamente perché non sempre presente negli header di sistema.
 *
 * @see https://www.ibm.com/docs/it/aix/7.3.0?topic=s-semctl-subroutine
 */
union semun {
  int val;
  struct semid_ds *buf;
  unsigned short *array;
};

#define IPC_FILENAME "servizio_mensa_ipc.lock"

#define FTOK_SHM_ID 'M'
#define FTOK_SEM_ID 'S'
#define FTOK_MSG_ID 'Q'
#define FTOK_SHM_ROLES_ID 'R'
#define FTOK_SHM_SUPPLY_ID 'K'

/**
 * @brief Genera e restituisce la chiave IPC del progetto.
 *
 * Utilizza ftok() su un file lock condiviso.
 *
 * @param project_id Identificativo del progetto (es. 'M', 'S', 'Q')
 * @return Chiave IPC valida, oppure -1 in caso di errore
 */
key_t get_project_ipc_key(int project_id);

/////////////////////
//  SHARED MEMORY  //
/////////////////////

/**
 * @brief Crea o ottiene un segmento di memoria condivisa.
 *
 * @param size Dimensione del segmento in byte
 * @param project_id Identificativo del progetto ['M', 'S', 'Q', 'R']
 * @return ID del segmento di memoria condivisa, -1 in caso di errore
 */
int allocate_shm(size_t size, int project_id);

/**
 * @brief Attacca un segmento di memoria condivisa al processo.
 *
 * @param shm_id ID del segmento di memoria condivisa
 * @return Puntatore alla memoria condivisa, NULL in caso di errore
 */
void *attach_shm(int shm_id);

/**
 * @brief Stacca un segmento di memoria condivisa dal processo.
 *
 * @param ptr Puntatore precedentemente restituito da attach_shm()
 * @return 0 in caso di successo, -1 in caso di errore
 */
int detach_shm(void *ptr);

/**
 * @brief Marca un segmento di memoria condivisa per la rimozione.
 *
 * La rimozione effettiva avviene quando nessun processo è più attaccato.
 *
 * @param shm_id ID del segmento
 * @return 0 in caso di successo, -1 in caso di errore
 */
int remove_shm(int shm_id);

//////////////////
//  SEMAPHORES  //
//////////////////

/**
 * @brief Crea o ottiene un set di semafori.
 *
 * @param num_sems Numero di semafori nel set
 * @return ID del set di semafori, -1 in caso di errore
 */
int create_sem_set(int num_sems);

/**
 * @brief Inizializza il valore di un semaforo.
 *
 * @param sem_id ID del set di semafori
 * @param sem_num Indice del semaforo nel set
 * @param value Valore iniziale
 * @return 0 in caso di successo, -1 in caso di errore
 */
int init_sem(int sem_id, int sem_num, int value);

/**
 * @brief Operazione P (wait) su un semaforo.
 *
 * Decrementa il semaforo e blocca il processo se il valore è 0.
 *
 * @param sem_id ID del set di semafori
 * @param sem_num Indice del semaforo
 * @return 0 in caso di successo, -1 in caso di errore
 */
int sem_wait(int sem_id, int sem_num);

/**
 * @brief Operazione V (signal) su un semaforo.
 *
 * Incrementa il semaforo e risveglia eventuali processi bloccati.
 *
 * @param sem_id ID del set di semafori
 * @param sem_num Indice del semaforo
 * @return 0 in caso di successo, -1 in caso di errore
 */
int sem_signal(int sem_id, int sem_num);

/**
 * @brief Rimuove un intero set di semafori.
 *
 * @param sem_id ID del set
 * @return 0 in caso di successo, -1 in caso di errore
 */
int remove_sem_set(int sem_id);

//////////////////////
//  MESSAGE QUEUES  //
//////////////////////

/**
 * @brief Crea o ottiene una coda di messaggi.
 *
 * @return ID della coda di messaggi, -1 in caso di errore
 */
int create_msg_queue(void);

/**
 * @brief Invia un messaggio su una coda.
 *
 * @param msg_id ID della coda
 * @param msg Puntatore alla struttura del messaggio
 * @param size Dimensione del payload (escluso mtype)
 * @param flags Flag IPC (es. IPC_NOWAIT)
 * @return 0 in caso di successo, -1 in caso di errore
 */
int send_message(int msg_id, void *msg, size_t size, int flags);

/**
 * @brief Riceve un messaggio da una coda.
 *
 * @param msg_id ID della coda
 * @param msg Buffer di destinazione
 * @param size Dimensione massima del payload
 * @param msg_type Tipo di messaggio da ricevere
 * @param flags Flag IPC
 * @return Numero di byte ricevuti, -1 in caso di errore
 */
int receive_message(int msg_id, void *msg, size_t size, long msg_type,
                    int flags);

/**
 * @brief Rimuove una coda di messaggi.
 *
 * @param msg_id ID della coda
 * @return 0 in caso di successo, -1 in caso di errore
 */
int remove_msg_queue(int msg_id);

/**
 * @brief Acquisisce un mutex basato su semaforo.
 *
 * @param sem_id ID del set di semafori
 * @param sem_num Indice del semaforo mutex
 */
void sem_mutex_acquire(int sem_id, int sem_num);

/**
 * @brief Rilascia un mutex basato su semaforo.
 *
 * @param sem_id ID del set di semafori
 * @param sem_num Indice del semaforo mutex
 */
void sem_mutex_release(int sem_id, int sem_num);

#endif