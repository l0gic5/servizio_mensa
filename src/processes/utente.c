/**
 * @file utente.c
 * @brief Processo log_tag (Cliente).
 *
 * Simula il ciclo di vita di un cliente nella mensa:
 * 1. Decide il menù (Primi, Secondi, o entrambi + Caffè opzionale)
 * 2. Si mette in coda per le stazioni scelte
 * 3. Invia ordini tramite Message Queue e attende risposta
 * 4. Consuma il pasto (simulazione tempo)
 * 5. Paga alla cassa
 * 6. Gestisce casi di rinuncia (code piene) o terminazione anticipata
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
#include "common/stats.h"
#include "common/types.h"
#include "common/names.h"

static int g_sem_id = -1;
static int g_msg_id = -1;
static int g_shm_stats_id = -1;
static GlobalStats *g_stats = NULL;

static volatile sig_atomic_t g_running = 1;
static volatile sig_atomic_t g_day_ended = 0;

char *log_tag = "UTENTE";

/**
 * @brief Gestore segnali di terminazione
 */
void stop_handler(int sig) {
  (void)sig;
  g_running = 0;
}

/**
 * @brief Attende l'inizio della giornata (semaforo).
 */
void wait_for_day_start() {
  struct sembuf sb = {SEM_INDEX_DAY_CHANGE, -1, 0};
  if (semop(g_sem_id, &sb, 1) == -1) {
    if (errno != EINTR) {
      LOG_ERR(log_tag, "Errore attesa inizio giorno");
    }
  }
}

/**
 * @brief Gestore per il cambio giorno.
 * Serve solo a "svegliare" la pause() intercettando il segnale
 * invece di far terminare il processo.
 */
void day_change_handler(int sig) {
  (void)sig;
  g_day_ended = 1;
}

/**
 * @brief Segnala la fine della giornata ai processi Utente.
 */
void signal_end_of_day() {
  struct sembuf sb = {SEM_INDEX_BARRIER, 1, 0};
  semop(g_sem_id, &sb, 1);
}

/**
 * @brief Restituisce il timestamp corrente in secondi (con precisione
 * nanosecondi)
 */
double get_current_time_sec() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/**
 * @brief Aggiorna le statistiche dei tempi di attesa in SHM
 */
void update_wait_stats(OpType type, double wait_time) {
  sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);

  switch (type) {
  case OP_PRIMI:
    g_stats->total_wait_time_primi += wait_time;
    break;
  case OP_SECONDI:
    g_stats->total_wait_time_secondi += wait_time;
    break;
  case OP_CAFFE:
    g_stats->total_wait_time_caffe += wait_time;
    break;
  case OP_CASSA:
    g_stats->total_wait_time_cassa += wait_time;
    break;
  }

  sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);
}

/**
 * @brief Segnala che l'utente è stato servito con successo.
 */
void mark_as_served() {
  sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);
  g_stats->total_users_served++;
  sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);
}

/**
 * @brief Segnala che l'utente è stato rifiutato (code piene).
 */
void mark_as_refused() {
  sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);
  g_stats->total_users_refused++;
  sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);
}

/**
 * @brief Segnala che l'utente ha un ticket.
 */
void mark_as_w_ticket() {
  sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);
  g_stats->total_users_w_ticket++;
  sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);
}

/**
 * @brief Segnala che l'utente non ha potuto mangiare per mancanza di budget.
 */
void mark_as_poverty() {
  sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);
  g_stats->total_user_poverty++;
  sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);
}

/**
 * @brief Inizializza IPC collegandosi alle risorse esistenti
 */
int setup_ipc(void) {
  g_sem_id = create_sem_set(TOTAL_SEMS);
  g_msg_id = create_msg_queue();

  g_shm_stats_id = allocate_shm(sizeof(GlobalStats), FTOK_SHM_ID);
  g_stats = (GlobalStats *)attach_shm(g_shm_stats_id);

  if (g_sem_id == -1 || g_msg_id == -1 || g_stats == NULL) {
    return -1;
  }
  return 0;
}

/**
 * @brief Rilascia le risorse locali (detach SHM).
 */
void cleanup_resources() {
  if (g_stats) {
    detach_shm(g_stats);
  }

  free(log_tag);
  names_destroy();
}

/**
 * @brief Tenta di acquisire un posto in coda (Semaforo).
 *
 * Se la coda è piena (timeout), l'utente rinuncia.
 *
 * @param sem_index Indice del semaforo della coda.
 * @param queue_name Nome descrittivo della coda (per log).
 *
 * @return 0 se acquisito, -1 se rinuncia.
 */
int enter_queue(int sem_index, const char *queue_name, int timeout_sec) {
  if (!g_running) {
    return -1;
  }

  LOG_INFO(log_tag, "Tento accesso coda %s...", queue_name);

  struct sembuf sb;
  sb.sem_num = (unsigned short)sem_index;
  sb.sem_op = -1;
  sb.sem_flg = 0;

  struct timespec timeout;
  timeout.tv_sec = timeout_sec;
  timeout.tv_nsec = 0;

  // semtimedop => estensione GNU (richiede _GNU_SOURCE)
  if (semtimedop(g_sem_id, &sb, 1, &timeout) == -1) {

    // timeout scaduto (coda troppo lenta)
    if (errno == EAGAIN) {
      LOG_WARN(log_tag, "Coda %s troppo lenta! Rinuncio all'accesso.",
               queue_name);
      return -1;
    }

    // interruzione segnale (fine giornata)
    if (errno == EINTR) {
      LOG_WARN(log_tag, "Mensa chiusa mentre ero in coda %s!", queue_name);
      return -1;
    }

    LOG_ERR(log_tag, "Errore semtimedop su coda %s", queue_name);
    return -1;
  }

  return 0;
}

/**
 * @brief Esegue la transazione con un operatore o la cassa.
 *
 * @param type Tipo di operazione (Primi, Secondi, Caffè, Cassa).
 * @param msg_type Tipo di messaggio IPC da inviare.
 * @param amount Importo da pagare (0.0 se è un ordine di cibo, >0 se è
 * pagamento).
 *
 * @return 0 se servito con successo, -1 in caso di errore.
 */
int perform_order(OpType type, int msg_type, double amount, bool has_ticket) {
  if (!g_running) {
    return -1;
  }

  MessageRequest req;
  req.mtype = msg_type;
  req.sender_pid = getpid();
  req.total_cost = amount;

  req.food_choice[0] = (type == OP_PRIMI);
  req.food_choice[1] = (type == OP_SECONDI);
  req.food_choice[2] = (type == OP_CAFFE);
  req.wants_ticket = has_ticket;

  if (type == OP_CASSA) {
    LOG_INFO(log_tag, "Vado alla Cassa per pagare %.2f€...", amount);
  } else {
    LOG_INFO(log_tag, "Ordino %s...", ROLE_NAME(type));
  }

  if (send_message(g_msg_id, &req, REQ_PAYLOAD_SIZE, 0) == -1) {
    if (errno != EINTR) {
      LOG_ERR(log_tag, "Errore invio richiesta %s", ROLE_NAME(type));
    }
    return -1;
  }

  MessageResponse resp;
  // msgrcv bloccante su mtype = mio PID
  int bytes = receive_message(g_msg_id, &resp, RES_PAYLOAD_SIZE, getpid(), 0);

  if (bytes == -1) {
    // se fallisce (es. fine giornata mentre aspetto):
    // => logga solo se non è EINTR pulito
    if (errno != EINTR) {
      LOG_ERR(log_tag, "Nessuna risposta da %s", ROLE_NAME(type));
      // mark_as_refused();
    }
    return -1;
  }

  if (resp.status == ORDER_SOLD_OUT) {
    LOG_WARN(log_tag, "Operatore PID %d dice: Piatto Terminato!",
             resp.operator_pid);
    return -2;
  }

  if (type != OP_CASSA) {
    LOG_INFO(log_tag, "Ricevuto un %s da Operatore %d.",
             ROLE_NAME_SINGULAR(type), resp.operator_pid);
  }
  // ELSE scontrino stampato in user_routine dopo il return

  return 0;
}

/**
 * @brief Simula il consumo del pasto.
 * * Richiede un posto a sedere (Tavolo).
 */
void consume_meal(Config *cfg) {
  if (!g_running) {
    return;
  }

  LOG_INFO(log_tag, "Cerco tavolo...");
  if (sem_wait(g_sem_id, SEM_INDEX_TABLES) == -1) {
    return;
  }

  LOG_INFO(log_tag, "Mangio...");

  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = cfg->user_meal_duration_ns;

  nanosleep(&t, NULL);

  sem_signal(g_sem_id, SEM_INDEX_TABLES);
  LOG_INFO(log_tag, "Pasto finito, libero tavolo.");
}

/**
 * @brief Simula il consumo del pasto principale (Primi/Secondi).
 * Richiede un posto a sedere (Tavolo).
 *
 * @param cfg Configurazione globale.
 * @param has_food Indica se l'utente ha effettivamente del cibo da consumare.
 * In caso contrario, salta il consumo.
 */
void consume_main_meal(Config *cfg, bool has_food) {
  if (!g_running || !has_food) {
    return;
  }

  LOG_INFO(log_tag, "Cerco tavolo per mangiare...");
  if (sem_wait(g_sem_id, SEM_INDEX_TABLES) == -1) {
    return;
  }

  LOG_INFO(log_tag, "Mangio al tavolo...");
  struct timespec t = {0, cfg->user_meal_duration_ns};
  nanosleep(&t, NULL);

  sem_signal(g_sem_id, SEM_INDEX_TABLES);
  LOG_INFO(log_tag, "Pasto finito, libero tavolo.");
}

/**
 * @brief Simula il consumo del caffè al bancone.
 * Non richiede tavolo, tempo molto breve.
 */
void consume_coffee_bar(Config *cfg) {
  if (!g_running) {
    return;
  }

  LOG_INFO(log_tag, "Bevo caffè al bancone...");
  struct timespec t = {0, cfg->user_coffee_duration_ns};
  nanosleep(&t, NULL);
  LOG_INFO(log_tag, "Caffè finito.");
}

/**
 * @brief Loop giornaliero dell'utente.
 * 1. Prendi Primi/Secondi
 * 2. Paga (Cibo + Caffè prenotato)
 * 3. Mangia Cibo (Tavolo)
 * 4. Prendi Caffè (Se pagato) -> Bevi (Bancone)
 */
void user_routine(Config *cfg, double *current_budget, bool has_ticket) {
  double conto_da_pagare = 0.0;
  double budget_disponibile = *current_budget;

  bool wants_primo = false;
  bool wants_secondo = false;
  bool wants_caffe = false;

  bool got_primo = false;
  bool got_secondo = false;

  if (has_ticket && g_running && !g_day_ended) {
    if (enter_queue(SEM_INDEX_TICKET_READER, "TICKET_READER",
                    cfg->user_queue_timeout_sec) == 0) {
      LOG_INFO(log_tag, "Valido il ticket...");
      struct timespec t = {0, cfg->ticket_reader_timeout_ns};
      nanosleep(&t, NULL);
      sem_signal(g_sem_id, SEM_INDEX_TICKET_READER);
    } else {
      // timeout sul lettore ticket
      // => mangia senza sconto
      has_ticket = 0;
      LOG_WARN(log_tag,
               "Non sono riuscito a validare il ticket. Mangio senza sconto.");
    }
  }

  // 1) SCELTA MENU

  // secondo
  if (random_probability(cfg->probability_user_wants_secondo, rand)) {
    if (budget_disponibile >= cfg->price_secondi) {
      wants_secondo = true;
      budget_disponibile -= cfg->price_secondi;
      conto_da_pagare += cfg->price_secondi;
    }
  }

  // primo
  if (random_probability(cfg->probability_user_wants_primo, rand)) {
    if (budget_disponibile >= cfg->price_primi) {
      wants_primo = true;
      budget_disponibile -= cfg->price_primi;
      conto_da_pagare += cfg->price_primi;
    }
  }

  // caffè (prenotazione)
  if (random_probability(cfg->probability_user_wants_caffe, rand)) {
    if (budget_disponibile >= cfg->price_caffe) {
      wants_caffe = true;
      budget_disponibile -= cfg->price_caffe;
      conto_da_pagare += cfg->price_caffe;
    }
  }

  // fallback se non ha scelto nulla ma ha budget per caffè
  if (!wants_primo && !wants_secondo && !wants_caffe &&
      budget_disponibile >= cfg->price_caffe) {
    wants_caffe = true;
    budget_disponibile -= cfg->price_caffe;
    conto_da_pagare += cfg->price_caffe;
  }

  if (conto_da_pagare <= 0.001) {
    mark_as_poverty();
    LOG_WARN(log_tag, "Oggi troppo povero (ho solo %.2f€). Salto il pasto.",
             *current_budget);
    return;
  }

  // 2) PRELIEVO CIBO (primi / secondi)
  // primi
  if (wants_primo && g_running && !g_day_ended) {
    double start = get_current_time_sec();
    if (enter_queue(SEM_INDEX_SEATS_PRIMI, "PRIMI",
                    cfg->user_queue_timeout_sec) == 0) {
      int res = perform_order(OP_PRIMI, MSG_TYPE_ORDER_PRIMI, 0.0, has_ticket);

      if (res == 0) {
        got_primo = true;
        update_wait_stats(OP_PRIMI, get_current_time_sec() - start);
      } else if (res == -2) {
        // storno budget
        *current_budget += cfg->price_primi;
        conto_da_pagare -= cfg->price_primi;

        if (!wants_secondo && *current_budget >= cfg->price_secondi) {
          LOG_INFO(log_tag, "Primo finito. Ripiego su SECONDO.");
          wants_secondo = true;
          *current_budget -= cfg->price_secondi;
          conto_da_pagare += cfg->price_secondi;
        }
      }
      sem_signal(g_sem_id, SEM_INDEX_SEATS_PRIMI);
    } else {
      *current_budget += cfg->price_primi;
      conto_da_pagare -= cfg->price_primi;
    }
  }

  // secondi
  if (wants_secondo && g_running && !g_day_ended) {
    double start = get_current_time_sec();
    if (enter_queue(SEM_INDEX_SEATS_SECONDI, "SECONDI",
                    cfg->user_queue_timeout_sec) == 0) {
      int res =
          perform_order(OP_SECONDI, MSG_TYPE_ORDER_SECONDI, 0.0, has_ticket);

      if (res == 0) {
        got_secondo = true;
        update_wait_stats(OP_SECONDI, get_current_time_sec() - start);
      } else if (res == -2) {
        *current_budget += cfg->price_secondi;
        conto_da_pagare -= cfg->price_secondi;

        // fallback caffè
        if (!wants_caffe && *current_budget >= cfg->price_caffe) {
          LOG_INFO(log_tag, "Secondo finito. Ripiego su CAFFE.");
          wants_caffe = true;
          *current_budget -= cfg->price_caffe;
          conto_da_pagare += cfg->price_caffe;
        }
      }
      sem_signal(g_sem_id, SEM_INDEX_SEATS_SECONDI);
    } else {
      *current_budget += cfg->price_secondi;
      conto_da_pagare -= cfg->price_secondi;
    }
  }

  if (!got_primo && !got_secondo && !wants_caffe && !g_day_ended) {
    mark_as_refused();
    LOG_INFO(log_tag, "Oggi non mangio niente. Esco.");
    return;
  }

  // 3) PAGAMENTO ALLA CASSA

  bool paid = false;

  double importo_effettivo = conto_da_pagare;
  if (has_ticket) {
    double sconto = conto_da_pagare * (cfg->ticket_discount_percent / 100.0);
    importo_effettivo -= sconto;
  }

  if (g_running && conto_da_pagare > 0.001 && !g_day_ended) {

    if (*current_budget < importo_effettivo) {
      mark_as_poverty();
      LOG_WARN(log_tag, "Budget insufficiente anche con sconto. Esco.");
      return;
    }

    double start = get_current_time_sec();
    if (enter_queue(SEM_INDEX_SEATS_CASSA, "CASSA",
                    cfg->user_queue_timeout_sec) == 0) {

      if (perform_order(OP_CASSA, MSG_TYPE_PAYMENT, importo_effettivo,
                        has_ticket) != -1) {
        // Addebito effettivo
        *current_budget -= importo_effettivo;

        update_wait_stats(OP_CASSA, get_current_time_sec() - start);

        if (has_ticket) {
          mark_as_w_ticket();
        }
        mark_as_served();

        char *log_msg = has_ticket ? "scontato ticket" : "prezzo intero";

        LOG_INFO(log_tag, "Pagato %.2f€ [%s]", importo_effettivo, log_msg);
        paid = true;
      }
      sem_signal(g_sem_id, SEM_INDEX_SEATS_CASSA);
    }
  }

  // sciopero cassa o timeout
  if (!paid && conto_da_pagare > 0.001) {
    mark_as_refused();
    LOG_WARN(log_tag, "Impossibile pagare. Abbandono il vassoio ed esco.");
    return;
  }

  // 4) CONSUMO PASTO (Tavolo)

  if ((got_primo || got_secondo) && g_running && !g_day_ended) {
    consume_main_meal(cfg, true);
  }

  // 5) CAFFÈ

  if (wants_caffe && g_running && !g_day_ended) {
    double start = get_current_time_sec();

    if (enter_queue(SEM_INDEX_SEATS_CAFFE, "CAFFE",
                    cfg->user_queue_timeout_sec) == 0) {
      int res = perform_order(OP_CAFFE, MSG_TYPE_ORDER_CAFFE, 0.0, has_ticket);

      if (res == 0) {
        update_wait_stats(OP_CAFFE, get_current_time_sec() - start);

        consume_coffee_bar(cfg);
      } else if (res == -2) {
        // TEORICAMENTE non dovrebbe succedere => caffè infinito (soglia molto
        // alta)
        LOG_WARN(log_tag, "Caffè finito! Ho pagato per nulla :(");
      }
      sem_signal(g_sem_id, SEM_INDEX_SEATS_CAFFE);
    } else {
      LOG_WARN(log_tag, "Coda caffè impossibile. Rinuncio al caffè pagato.");
    }
  }
}

int main(int argc, char *argv[]) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = stop_handler;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  struct sigaction sa_usr;
  memset(&sa_usr, 0, sizeof(sa_usr));
  sa_usr.sa_handler = day_change_handler;
  sigaction(SIGUSR1, &sa_usr, NULL);

  srand((unsigned int)time(NULL) ^ (unsigned int)getpid());

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <config_path> <has_ticket>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  const char *config_path =
      (argv[1][0] != '\0') ? argv[1] : "conf/default.conf";

  char *endptr;
  long temp_val = strtol(argv[2], &endptr, 10);
  bool has_ticket = (endptr == argv[2] || *endptr != '\0' || temp_val == -1)
                        ? true
                        : (bool)temp_val;

  names_init();
  log_tag = get_random_identity(ROLE_UTENTE);

  Config config;
  if (parse_config(config_path, &config) == -1) {
    exit(EXIT_FAILURE);
  }

  if (setup_ipc() == -1) {
    exit(EXIT_FAILURE);
  }

  double my_budget =
      random_range(config.user_budget_min, config.user_budget_max, rand);

  LOG_INFO(log_tag, "Cliente arrivato in mensa. Patrimonio iniziale: %.2f€",
           my_budget);

  for (int day = 1; day <= config.simulation_duration_days && g_running;
       day++) {

    wait_for_day_start();

    // reset giornaliero: flag alzato dal SIGUSR1 (fine giornata)
    // senza questo => non fa più pause() e segnala la barriera anche quando il
    // Responsabile non ha chiuso
    g_day_ended = 0;

    // ritardo casuale arrivo utente
    if (config.user_max_arrival_delay_us > 0) {
      struct timespec ts = {
          0,
          (long)random_range(0, config.user_max_arrival_delay_us, rand) * 1000};
      if (nanosleep(&ts, NULL) == -1 && !g_running) {
        break;
      }
    }

    double daily_salary = random_range(config.user_min_daily_salary,
                                       config.user_max_daily_salary, rand);
    my_budget += daily_salary;

    if (my_budget > config.user_budget_max) {
      my_budget = config.user_budget_max;
    }

    LOG_INFO(log_tag, "Giorno %d: Ricevuto stipendio %.2f€. Totale: %.2f€",
             day, daily_salary, my_budget);

    if (config.user_max_arrival_delay_us > 0) {
      struct timespec ts = {
          0,
          (long)random_range(0, config.user_max_arrival_delay_us, rand) * 1000};
      if (nanosleep(&ts, NULL) == -1 && !g_running) {
        break;
      }
    }

    user_routine(&config, &my_budget, has_ticket);

    LOG_INFO(log_tag, "Finito il pasto, attendo chiusura mensa (Giorno %d)...",
             day);

    // race condition fixed!
    if (g_running) {
      sigset_t mask, old_mask;
      sigemptyset(&mask);
      sigaddset(&mask, SIGUSR1);

      // blocco SIGUSR1
      sigprocmask(SIG_BLOCK, &mask, &old_mask);

      // segnale è già arrivato prima che bloccassi o è arrivato mentre stavo
      // bloccando?
      if (!g_day_ended) {
        // se giorno non è ancora finito, mi metto in attesa.
        sigsuspend(&old_mask);
      }

      sigprocmask(SIG_SETMASK, &old_mask, NULL);
    }

    signal_end_of_day();

    // Quando arriva SIGUSR1, handle_day_end viene chiamato (vuoto o flag),
    // pause() si sblocca e il ciclo ricomincia.
  }

  return 0;
}