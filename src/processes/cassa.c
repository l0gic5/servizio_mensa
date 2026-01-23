/**
 * @file cassa.c
 * @brief Processo Cassa.
 *
 * Il processo Cassa è responsabile della gestione dei pagamenti.
 * - Attende messaggi di tipo MSG_TYPE_PAYMENT dalla coda messaggi.
 * - Simula il tempo di transazione (lettura badge, scontrino, resto).
 * - Aggiorna il ricavo totale nelle statistiche condivise (in mutua
 * esclusione).
 * - Invia una conferma di pagamento al cliente sbloccandolo.
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "common/config.h"
#include "common/ipc_utils.h"
#include "common/logger.h"
#include "common/names.h"
#include "common/stats.h"
#include "common/types.h"

static int g_shm_stats_id = -1;
static int g_shm_roles_id = -1;
static int g_msg_id = -1;
static int g_sem_id = -1;

static GlobalStats *g_stats = NULL;
static WorkerConfig *g_worker_config = NULL;

static volatile sig_atomic_t g_running = 1;
static volatile sig_atomic_t g_day_signal = 0;

char *log_tag = "CASSA";

/**
 * @brief Gestore per terminazione pulita (SIGINT, SIGTERM).
 */
void cleanup_handler(int sig) {
  (void)sig;
  g_running = 0;
}

/**
 * @brief Gestore per il segnale di cambio giorno (SIGUSR1).
 *
 * @param sig Signal number
 */
void day_handler(int sig) {
  (void)sig;
  g_day_signal = 1;
}

/**
 * @brief Rilascia le risorse locali (detach SHM).
 */
void cleanup_resources(void) {
  if (g_stats) {
    detach_shm(g_stats);
  }
  if (g_worker_config) {
    detach_shm(g_worker_config);
  }

  free(log_tag);
  names_destroy();

  LOG_INFO(log_tag, "Chiusura modulo cassa.");
}

/**
 * @brief Inizializza IPC collegandosi alle risorse del Responsabile.
 */
int setup_ipc(void) {
  // statistiche SHM
  g_shm_stats_id = allocate_shm(sizeof(GlobalStats), FTOK_SHM_ID);
  if (g_shm_stats_id == -1) {
    return -1;
  }
  g_stats = (GlobalStats *)attach_shm(g_shm_stats_id);
  if (g_stats == NULL) {
    return -1;
  }

  g_shm_roles_id = allocate_shm(sizeof(WorkerConfig), FTOK_SHM_ROLES_ID);
  if (g_shm_roles_id == -1) {
    return -1;
  }
  g_worker_config = (WorkerConfig *)attach_shm(g_shm_roles_id);
  if (g_worker_config == NULL) {
    return -1;
  }

  // coda messaggi
  g_msg_id = create_msg_queue();
  if (g_msg_id == -1) {
    return -1;
  }

  // semafori
  g_sem_id = create_sem_set(TOTAL_SEMS);
  if (g_sem_id == -1) {
    return -1;
  }

  return 0;
}

int main(int argc, char *argv[]) {
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sa.sa_handler = cleanup_handler;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  sa.sa_handler = day_handler;
  sigaction(SIGUSR1, &sa, NULL);

  srand((unsigned int)time(NULL) ^ (unsigned int)getpid());

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <id> <config_path>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  int my_id = atoi(argv[1]);

  names_init();
  log_tag = get_random_identity(ROLE_CASSA);

  Config config;
  if (parse_config(argv[2], &config) == -1) {
    LOG_ERR(log_tag, "Errore parsing config");
    exit(EXIT_FAILURE);
  }

  if (setup_ipc() == -1) {
    LOG_ERR(log_tag, "Errore connessione IPC");
    exit(EXIT_FAILURE);
  }

  sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);
  strncpy(g_worker_config->worker_names[my_id], log_tag, 63);
  g_worker_config->worker_names[my_id][63] = '\0';
  sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);

  LOG_INFO(log_tag, "Modulo avviato. In attesa di pagamenti...");

  bool queue_error = false;
  while (g_running && !queue_error) {
    time_t now = time(NULL);
    if (g_worker_config->strike_end_times[my_id] > now) {
      double duration = difftime(g_worker_config->strike_end_times[my_id], now);

      LOG_INFO(log_tag,
               COLOR_RED "SCIOPERO! Cassa chiusa per %.0f s." COLOR_RESET,
               duration);

      struct timespec req = {(time_t)duration, 0};

      nanosleep(&req, NULL);

      LOG_INFO(log_tag, "Riapertura cassa.");
    }

    // SE arriva segnale di fine giornata, segnalo la barriera anche
    // nel caso in cui la coda non sia mai vuota
    // (msgrcv non blocca => niente EINTR).
    // SCRIVE (sem_op = +1) sul semaforo barriera SEM_INDEX_BARRIER per
    // notificare al Responsabile che la Cassa ha terminato le operazioni.
    // Il Responsabile LEGGE questo semaforo in responsabile.c:handle_day_end_sync().
    if (g_day_signal) {
      struct sembuf sb = {SEM_INDEX_BARRIER, 1, 0};
      semop(g_sem_id, &sb, 1);
      g_day_signal = 0;

    } else {
      MessageRequest req;

      // ricezione bloccante di messaggi di tipo PAYMENT
      // SE coda vuota => il processo dorme
      ssize_t bytes = receive_message(g_msg_id, &req, REQ_PAYLOAD_SIZE,
                                      MSG_TYPE_PAYMENT, 0);

      if (bytes > 0) {
        long srv_time =
            (long)random_variance((double)config.avg_service_cassa,
                                  (double)config.variability_cassa, rand);

        if (srv_time < 0) {
          srv_time = 0;
        }

        struct timespec t = {0, srv_time};
        nanosleep(&t, NULL);

        // statistiche (MUTual EXclusion)
        sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);
        g_stats->total_revenue += req.total_cost;
        g_stats->total_transactions++;
        sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);

        char *log_msg = req.wants_ticket ? "scontato ticket" : "prezzo intero";
        LOG_INFO(log_tag, "Incasso: %.2f€ [%s] (Cliente PID %d)",
                 req.total_cost, log_msg, req.sender_pid);

        MessageResponse resp;
        resp.mtype = req.sender_pid;
        resp.operator_pid = getpid();
        resp.status = ORDER_SUCCESS;
        send_message(g_msg_id, &resp, RES_PAYLOAD_SIZE, 0);
      } else if (bytes == -1) {
        if (errno == EINTR && g_day_signal) {
          // Segnale di fine giornata ricevuto durante msgrcv bloccante.
          // SCRIVE (sem_op = +1) sul semaforo barriera SEM_INDEX_BARRIER per
          // notificare al Responsabile che la Cassa ha terminato le operazioni.
          // Il Responsabile LEGGE questo semaforo in responsabile.c:handle_day_end_sync().
          struct sembuf sb = {SEM_INDEX_BARRIER, 1, 0};
          semop(g_sem_id, &sb, 1);

          g_day_signal = 0;

        } else if (errno != EINTR && g_running) {
          LOG_ERR(log_tag, "Errore critico msgrcv");
          queue_error = true;
        }
      }
    }
  }

  cleanup_resources();
  return 0;
}