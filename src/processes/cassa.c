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
#include "common/types.h"

static int g_shm_stats_id = -1;
static int g_msg_id = -1;
static int g_sem_id = -1;

static Statistics *g_stats = NULL;

static volatile sig_atomic_t g_running = 1;

/**
 * @brief Gestore per terminazione pulita (SIGINT, SIGTERM).
 */
void cleanup_handler(int sig) {
  (void)sig;
  g_running = 0;
}

/**
 * @brief Gestore per il segnale di fine giornata (SIGUSR1).
 *
 * La cassa non chiude tra un giorno e l'altro (rimane attiva nel loop),
 * ma il segnale serve a interrompere msgrcv se bloccata, permettendo
 * eventuali controlli o log di fine giornata se necessari.
 */
void day_handler(int sig) { (void)sig; }

/**
 * @brief Rilascia le risorse locali (detach SHM).
 */
void cleanup_resources(void) {
  if (g_stats) {
    detach_shm(g_stats);
  }
  LOG_INFO("CASSA", "Chiusura modulo cassa.");
}

/**
 * @brief Inizializza IPC collegandosi alle risorse del Responsabile.
 */
int setup_cassa_ipc(void) {
  // statistiche SHM
  g_shm_stats_id = allocate_shm(sizeof(Statistics), FTOK_SHM_ID);
  if (g_shm_stats_id == -1) {
    return -1;
  }
  g_stats = (Statistics *)attach_shm(g_shm_stats_id);

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

  Config config;
  if (parse_config(argv[2], &config) == -1) {
    LOG_ERR("CASSA", "Errore parsing config");
    exit(EXIT_FAILURE);
  }

  if (setup_cassa_ipc() == -1) {
    LOG_ERR("CASSA", "Errore connessione IPC");
    exit(EXIT_FAILURE);
  }

  LOG_INFO("CASSA", "Modulo avviato. In attesa di pagamenti...");

  bool queue_error = false;
  while (g_running && !queue_error) {
    MessageRequest req;

    // ricezione bloccante di messaggi di tipo PAYMENT
    // SE coda vuota => il processo dorme
    ssize_t bytes =
        receive_message(g_msg_id, &req, REQ_PAYLOAD_SIZE, MSG_TYPE_PAYMENT, 0);

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
      if (sem_wait(g_sem_id, SEM_INDEX_MUTEX_STATS) != -1) {
        g_stats->total_revenue += req.total_cost;
        sem_signal(g_sem_id, SEM_INDEX_MUTEX_STATS);
      }

      LOG_INFO("CASSA", "Incasso: %.2f€ (Cliente PID %d)", req.total_cost,
               req.sender_pid);

      MessageResponse resp;
      resp.mtype = req.sender_pid;
      resp.operator_pid = getpid();
      send_message(g_msg_id, &resp, RES_PAYLOAD_SIZE, 0);
    } else {
      // SE errore NON è EINTR (segnale interruzione)
      // => errore vero coda messaggi
      if (errno != EINTR && g_running) {
        LOG_ERR("CASSA", "Errore critico msgrcv");
        queue_error = true;
      }
      // errno == EINTR => continue loop
    }
  }

  cleanup_resources();
  return 0;
}