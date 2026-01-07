/**
 * @file operatore.c
 * @brief Processo "Operatore" (Worker).
 *
 * Questo file contiene il codice sorgente per i processi lavoratori.
 * Ogni operatore simula il comportamento di un dipendente della mensa:
 * - Recupera il proprio ruolo dinamico dalla Shared Memory
 * - Compete per una postazione di lavoro (semafori)
 * - Preleva richieste dalla coda messaggi
 * - Simula il tempo di servizio
 * - Invia risposte agli Utenti
 * - Gestisce le pause in base alla configurazione
 * - Aggiorna le statistiche e gestisce le pause
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <time.h>
#include <unistd.h>

#include "common/config.h"
#include "common/ipc_utils.h"
#include "common/logger.h"
#include "common/types.h"

static int g_shm_stats_id = -1;
static int g_shm_roles_id = -1;
static int g_shm_supply_id = -1;
static int g_msg_id = -1;
static int g_sem_id = -1;

static GlobalStats *g_stats = NULL;
static KitchenState *g_kitchen = NULL;
static WorkerConfig *g_worker_config = NULL;

static volatile sig_atomic_t g_running = 1;
/// Flag volatile per indicare la fine della giornata lavorativa (impostato da
/// signal handler)
static volatile sig_atomic_t g_day_ended = 0;

/**
 * @brief Gestore dei segnali di terminazione (SIGTERM, SIGINT).
 *
 * Esegue il detach dalle memorie condivise e termina il processo.
 *
 * @param sig Il numero del segnale ricevuto.
 */
void cleanup_resources(void) {
  if (g_stats) {
    detach_shm(g_stats);
  }
  if (g_kitchen) {
    detach_shm(g_kitchen);
  }
  if (g_worker_config) {
    detach_shm(g_worker_config);
  }

  LOG_INFO("OPERATORE", "Chiusura operatore...");
  exit(0);
}

/**
 * @brief Gestore segnali di terminazione (SIGTERM, SIGINT).
 * Si limita a settare il flag, niente printf o exit qui.
 */
void stop_handler(int sig) {
  (void)sig;
  g_running = 0;
}

/**
 * @brief Gestore del segnale di fine giornata (SIGUSR1).
 *
 * Imposta il flag globale `g_day_ended` a 1 per permettere l'uscita
 * pulita dal ciclo di servizio.
 *
 * @param sig Il numero del segnale ricevuto.
 */
void handle_day_end(int sig) {
  (void)sig;
  g_day_ended = 1;
}

/**
 * @brief Segnala al Responsabile la fine della giornata lavorativa.
 *
 * Utilizza un'operazione di signal sul semaforo di barriera per
 * notificare al Responsabile che questo operatore ha completato
 * il proprio turno giornaliero.
 */
void signal_end_of_day() {
  struct sembuf sb = {SEM_INDEX_BARRIER, 1, 0};
  semop(g_sem_id, &sb, 1);
}

/**
 * @brief Inizializza le risorse IPC specifiche per l'operatore.
 *
 * Si collega alle memorie condivise (Stats e Roles), alla coda di messaggi
 * e al set di semafori già creati dal processo Responsabile.
 *
 * @return 0 in caso di successo, -1 in caso di errore.
 */
int setup_ipc(void) {
  // 1) SHM statistiche
  g_shm_stats_id = allocate_shm(sizeof(GlobalStats), FTOK_SHM_ID);
  if (g_shm_stats_id == -1)
    return -1;
  g_stats = (GlobalStats *)attach_shm(g_shm_stats_id);

  g_shm_supply_id = allocate_shm(sizeof(KitchenState), FTOK_SHM_SUPPLY_ID);
  if (g_shm_supply_id == -1)
    return -1;
  g_kitchen = (KitchenState *)attach_shm(
      g_shm_supply_id); // Nota: g_kitchen, non g_supply

  // 2) SHM ruoli operatori
  g_shm_roles_id = allocate_shm(sizeof(WorkerConfig), FTOK_SHM_ROLES_ID);
  if (g_shm_roles_id == -1)
    return -1;
  g_worker_config = (WorkerConfig *)attach_shm(g_shm_roles_id);

  // coda messaggi & semafori
  g_msg_id = create_msg_queue();
  g_sem_id = create_sem_set(TOTAL_SEMS);

  if (g_msg_id == -1 || g_sem_id == -1) {
    return -1;
  }
  return 0;
}

/**
 * @brief Calcola un tempo di servizio casuale.
 *
 * Genera un tempo di attesa basato sulla media fornita e una percentuale
 * di variabilità.
 *
 * @param avg_ns Tempo medio di servizio in nanosecondi.
 * @param range_percent Percentuale di variabilità (es. 50 = +/- 50%).
 *
 * @return Il tempo di servizio calcolato (in nanosecondi).
 */
long calculate_service_time(long avg_ns, int range_percent) {
  long time_with_variance =
      (long)random_variance((double)avg_ns, (double)range_percent, rand);

  return (time_with_variance > 0) ? time_with_variance : 0;
}

/**
 * @brief Configura i parametri operativi in base al ruolo assegnato.
 *
 * Questa funzione mappa l'enum `OpType` ai valori specifici di configurazione
 * necessari per il loop di servizio (tempi, indici semafori, tipi messaggio).
 *
 * @param role Il ruolo corrente dell'operatore.
 * @param config Puntatore alla configurazione globale.
 * @param[out] avg_service_time Puntatore per restituire il tempo medio.
 * @param[out] sem_workstation_index Puntatore per restituire l'indice del
 * semaforo postazione.
 * @param[out] msg_type_req Puntatore per restituire il tipo di messaggio
 * atteso.
 * @param[out] range_percent Puntatore per restituire la variabilità del tempo.
 */
void set_role_parameters(OpType role, const Config *config,
                         long *avg_service_time, int *sem_workstation_index,
                         int *msg_type_req, int *range_percent) {
  switch (role) {
  case OP_PRIMI:
    *avg_service_time = config->avg_service_primi;
    *sem_workstation_index = SEM_OPERATORS_PRIMI;
    *msg_type_req = MSG_TYPE_ORDER_PRIMI;
    *range_percent = config->variability_primi;
    break;
  case OP_SECONDI:
    *avg_service_time = config->avg_service_secondi;
    *sem_workstation_index = SEM_OPERATORS_SECONDI;
    *msg_type_req = MSG_TYPE_ORDER_SECONDI;
    *range_percent = config->variability_secondi;
    break;
  case OP_CAFFE:
    *avg_service_time = config->avg_service_caffe;
    *sem_workstation_index = SEM_OPERATORS_CAFFE;
    *msg_type_req = MSG_TYPE_ORDER_CAFFE;
    *range_percent = config->variability_caffe;
    break;
  case OP_CASSA:
    *avg_service_time = config->avg_service_cassa;
    *sem_workstation_index = SEM_OPERATORS_CASSA;
    *msg_type_req = MSG_TYPE_PAYMENT;
    *range_percent = config->variability_cassa;
    break;
  default:
    LOG_ERR("OPERATORE", "Ruolo sconosciuto: %s", ROLE_NAME(role));
    exit(EXIT_FAILURE);
  }
}

/**
 * @brief Tenta di effettuare una pausa lavorativa.
 *
 * Controlla se il numero massimo di pause è stato raggiunto. In caso contrario,
 * valuta una probabilità di pausa. Se la pausa avviene, l'operatore rilascia
 * la postazione (signal), va in sleep, e poi tenta di riacquisirla (wait).
 *
 * @note **TOCTOU (Time Of Check to Time Of Use):** L'uso di valori statistici
 * o controlli non atomici prima di un'azione in concorrenza può generare
 * race conditions. In questa simulazione didattica, il rischio viene messo in
 * conto e tollerato...
 *
 * @param sem_id ID del set di semafori.
 * @param sem_workstation_index Indice del semaforo della postazione corrente.
 * @param[in,out] pauses_done Puntatore al contatore delle pause effettuate.
 * @param role Ruolo dell'operatore (per logging).
 * @param config Configurazione globale (per durate e limiti).
 */
void attempt_pause(int sem_id, int sem_workstation_index, int *pauses_done,
                   OpType role, Config config) {
  if (*pauses_done >= config.max_pauses_per_day) {
    return;
  }

  // `pause_probability_percent` di probabilità di fare pausa
  if (random_probability(config.pause_probability_percent, rand)) {
    // TOCTOU (Time Of Check to Time Of Use) => possibile Race Condition !!
    // ACCETTABILE !!
    // == il processo potrebbe essere prelevato dalla CPU tra il check e il wait

    if (sem_wait(sem_id, SEM_INDEX_MUTEX_STATS) == -1) {
      return;
    }

    int active = 0;
    switch (role) {
    case OP_PRIMI:
      active = g_worker_config->active_primi;
      break;
    case OP_SECONDI:
      active = g_worker_config->active_secondi;
      break;
    case OP_CAFFE:
      active = g_worker_config->active_caffe;
      break;
    case OP_CASSA:
      // superfluo, ma per coerenza
      active = g_worker_config->active_cassa;
      break;
    }

    // SE ultimo rimasto (active <= 1), niente pausa
    if (active <= 1) {
      sem_signal(sem_id, SEM_INDEX_MUTEX_STATS);
      LOG_INFO("OPERATORE", "Pausa negata: unico operatore attivo per %s",
               ROLE_NAME(role));
      return;
    }

    switch (role) {
    case OP_PRIMI:
      g_worker_config->active_primi--;
      break;
    case OP_SECONDI:
      g_worker_config->active_secondi--;
      break;
    case OP_CAFFE:
      g_worker_config->active_caffe--;
      break;
    case OP_CASSA:
      // superfluo, ma per coerenza
      g_worker_config->active_cassa--;
      break;
    }
    sem_signal(sem_id, SEM_INDEX_MUTEX_STATS);

    LOG_INFO("OPERATORE", "Pausa %d/%d (Ruolo %s)", *pauses_done + 1,
             config.max_pauses_per_day, ROLE_NAME(role));

    // lascia il posto alla workstation
    if (sem_signal(sem_id, sem_workstation_index) == -1) {
      return;
    }

    // wait
    struct timespec t_pause = {0, (long)config.pause_duration_ns};
    nanosleep(&t_pause, NULL);
    (*pauses_done)++;

    LOG_INFO("OPERATORE", "Fine pausa. Attendo postazione...");

    // Attendo postazione fisica per rientrare
    while (g_running) {
      if (sem_wait(sem_id, sem_workstation_index) == 0) {
        break;
      }
      if (errno != EINTR) {
        g_running = 0;
        break;
      }
    }

    // fine pausa
    sem_wait(sem_id, SEM_INDEX_MUTEX_STATS);

    switch (role) {
    case OP_PRIMI:
      g_worker_config->active_primi++;
      break;
    case OP_SECONDI:
      g_worker_config->active_secondi++;
      break;
    case OP_CAFFE:
      g_worker_config->active_caffe++;
      break;
    case OP_CASSA:
      // superfluo, ma per coerenza
      g_worker_config->active_cassa++;
      break;
    }

    sem_signal(sem_id, SEM_INDEX_MUTEX_STATS);

    LOG_INFO("OPERATORE", "Rientrato in servizio.");
  }
}

/**
 * @brief Ciclo principale di servizio giornaliero.
 *
 * Loop infinito (fino al segnale di fine giornata) che:
 * 1. Attende un messaggio (ordine) dalla coda IPC.
 * 2. Simula il tempo di preparazione (nanosleep).
 * 3. Invia una risposta al processo Utente.
 * 4. Aggiorna le statistiche globali in mutua esclusione.
 * 5. Tenta eventualmente una pausa.
 *
 * @param msg_id ID della coda messaggi.
 * @param sem_id ID del set di semafori.
 * @param sem_index Indice del semaforo postazione (per le pause).
 * @param avg_time Tempo medio di servizio.
 * @param msg_type Tipo di messaggio IPC da ascoltare.
 * @param range_p Percentuale di variabilità del servizio.
 * @param role Ruolo dell'operatore.
 * @param config Configurazione globale.
 */
void service_cycle(int msg_id, int sem_id, int sem_index, long avg_time,
                   int msg_type, int range_p, OpType role, Config config,
                   int my_id) {
  int pauses_done = 0;

  bool ended = false;
  while (!g_day_ended && !ended) {
    time_t now = time(NULL);
    time_t strike_end = g_worker_config->strike_end_times[my_id];

    if (strike_end > now) {
      double sleep_seconds = difftime(strike_end, now);

      LOG_WARN("OPERATORE", "SCIOPERO! Mi fermo per %.0f secondi.",
               sleep_seconds);

      struct timespec req = {(time_t)sleep_seconds, 0};
      struct timespec rem = {0, 0};

      if (nanosleep(&req, &rem) == -1 && errno == EINTR) {
        if (g_day_ended) {
          break;
        }
      }

      LOG_INFO("OPERATORE", "Sciopero terminato. Torno al lavoro.");
    }

    MessageRequest req;

    int res = receive_message(msg_id, &req, REQ_PAYLOAD_SIZE, msg_type, 0);

    if (res != -1) {
      // simulazione servizio
      OrderStatus order_status = ORDER_SOLD_OUT;

      // aggiornamento statistiche
      sem_wait(sem_id, SEM_INDEX_MUTEX_STATS);

      switch (role) {
      case OP_PRIMI:
        if (g_kitchen->remaining_primi > 0) {
          g_kitchen->remaining_primi--;
          g_stats->total_plates_primi++;
          g_stats->total_users_served++;
          order_status = ORDER_SUCCESS;
        }
        break;
      case OP_SECONDI:
        if (g_kitchen->remaining_secondi > 0) {
          g_kitchen->remaining_secondi--;
          g_stats->total_plates_secondi++;
          g_stats->total_users_served++;
          order_status = ORDER_SUCCESS;
        }
        break;
      case OP_CAFFE:
        if (g_kitchen->remaining_caffe > 0) {
          g_kitchen->remaining_caffe--;
          g_stats->total_plates_caffe++;
          g_stats->total_users_served++;
          order_status = ORDER_SUCCESS;
        }
        break;
      case OP_CASSA:
        g_stats->total_revenue += req.total_cost;
        g_stats->total_transactions++;
        order_status = ORDER_SUCCESS;
        break;
      default:
        break;
      }

      sem_signal(sem_id, SEM_INDEX_MUTEX_STATS);

      long srv_time = calculate_service_time(avg_time, range_p);
      struct timespec t = {0, srv_time};
      nanosleep(&t, NULL);

      MessageResponse resp;
      resp.mtype = req.sender_pid;
      resp.operator_pid = getpid();
      resp.status = order_status;

      send_message(msg_id, &resp, RES_PAYLOAD_SIZE, 0);

      // tentativo pausa
      attempt_pause(sem_id, sem_index, &pauses_done, role, config);
    }
    // se ricezione fallita => check se è per segnale di fine giornata
    else if (errno == EINTR && g_day_ended) {
      ended = true;
    }
  }
}

int main(int argc, char *argv[]) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = stop_handler;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  struct sigaction sa_day;
  memset(&sa_day, 0, sizeof(sa_day));
  sa_day.sa_handler = handle_day_end;
  sigaction(SIGUSR1, &sa_day, NULL);

  srand((unsigned int)time(NULL) ^ (unsigned int)getpid());

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <operator_id> <config_path>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int my_id = atoi(argv[1]);
  const char *config_path = argv[2];

  Config config;
  if (parse_config(config_path, &config) == -1) {
    LOG_ERR("OPERATORE", "Config parsing error: %s", config_path);
    exit(EXIT_FAILURE);
  }

  if (setup_ipc() == -1) {
    LOG_ERR("OPERATORE", "Errore IPC");
    exit(EXIT_FAILURE);
  }

  LOG_INFO("OPERATORE", "Avviato ID %d", my_id);

  // LOOP GIORNI
  for (int day = 1; day <= config.simulation_duration_days; day++) {

    // lettura dinamica del ruolo dalla SHM
    OpType my_role = g_worker_config->worker_roles[my_id];

    // configurazione parametri locali basati sul ruolo
    long avg_service_time;
    int sem_workstation_index;
    int msg_type_req;
    int range_percent;

    set_role_parameters(my_role, &config, &avg_service_time,
                        &sem_workstation_index, &msg_type_req, &range_percent);

    // 1) competizione per la Workstation
    LOG_INFO("OPERATORE", "Giorno %d: In coda per workstation ruolo %s...", day,
             ROLE_NAME(my_role));

    bool acquired = false;

    // loop di attesa | continua se:
    //   - non acquisisce
    //   - non c'è errore critico
    //   - la giornata non è finita
    while (g_running && !g_day_ended) {
      if (sem_wait(g_sem_id, sem_workstation_index) == 0) {
        acquired = true;
        break;
      }

      // Se l'errore NON è un segnale (EINTR), è un errore vero
      if (errno != EINTR) {
        LOG_ERR("OPERATORE", "Errore sem_wait workstation");
        g_running = 0;
        break;
      }

      // Se errno == EINTR, il loop ricomincia e controlla !g_day_ended nel
      // while
    }

    if (!g_running) {
      break;
    }

    // 2) ramificazione logica:
    // "ho lavorato o la giornata è finita mentre aspettavo"
    if (acquired) {
      LOG_INFO("OPERATORE", "Workstation acquisita. Inizio servizio.");

      service_cycle(g_msg_id, g_sem_id, sem_workstation_index, avg_service_time,
                    msg_type_req, range_percent, my_role, config, my_id);

      // rilascio postazione
      sem_signal(g_sem_id, sem_workstation_index);
    } else {
      // se non ho acquisito, significa che g_day_ended è diventato true mentre
      // ero in coda
      if (g_day_ended) {
        LOG_INFO("OPERATORE", "Giorno %d terminato (senza workstation).", day);
      }
    }

    // 3) sincronizzazione fine giornata (codice comune per entrambi i casi)
    //! reset flag per il giorno successivo
    g_day_ended = 0;

    signal_end_of_day();

    if (g_running) {
      LOG_INFO("OPERATORE", "Giorno %d terminato.", day);
    }
  }

  cleanup_resources();
  return 0;
}