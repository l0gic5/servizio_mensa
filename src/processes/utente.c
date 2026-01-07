/**
 * @file utente.c
 * @brief Processo "Utente" (Cliente).
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
#include "common/types.h"

static int g_sem_id = -1;
static int g_msg_id = -1;

static volatile sig_atomic_t g_running = 1;

/**
 * @brief Gestore segnali di terminazione
 */
void stop_handler(int sig) {
  (void)sig;
  g_running = 0;
}

/**
 * @brief Gestore per il cambio giorno.
 * Serve solo a "svegliare" la pause() intercettando il segnale
 * invece di far terminare il processo.
 */
void day_change_handler(int sig) { (void)sig; }

/**
 * @brief Inizializza IPC collegandosi alle risorse esistenti
 */
int setup_ipc(void) {
  g_sem_id = create_sem_set(TOTAL_SEMS);
  g_msg_id = create_msg_queue();

  if (g_sem_id == -1 || g_msg_id == -1) {
    return -1;
  }
  return 0;
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
  LOG_INFO("UTENTE", "Tento accesso coda %s...", queue_name);

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
      LOG_WARN("UTENTE", "Coda %s troppo lenta! Rinuncio al piatto.",
               queue_name);
      return -1;
    }

    // interruzione segnale (fine giornata)
    if (errno == EINTR) {
      return -1;
    }

    LOG_ERR("UTENTE", "Errore semtimedop su coda %s", queue_name);
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
int perform_order(OpType type, int msg_type, double amount) {
  MessageRequest req;
  req.mtype = msg_type;
  req.sender_pid = getpid();
  req.total_cost = amount;

  req.food_choice[0] = (type == OP_PRIMI);
  req.food_choice[1] = (type == OP_SECONDI);
  req.food_choice[2] = (type == OP_CAFFE);
  req.wants_ticket = 0;

  if (type == OP_CASSA) {
    LOG_INFO("UTENTE", "Vado alla Cassa per pagare %.2f€...", amount);
  } else {
    LOG_INFO("UTENTE", "Ordino %s...", ROLE_NAME(type));
  }

  if (send_message(g_msg_id, &req, REQ_PAYLOAD_SIZE, 0) == -1) {
    LOG_ERR("UTENTE", "Errore invio richiesta %s", ROLE_NAME(type));
    return -1;
  }

  MessageResponse resp;
  // msgrcv bloccante su mtype = mio PID
  int bytes = receive_message(g_msg_id, &resp, RES_PAYLOAD_SIZE, getpid(), 0);

  if (bytes == -1) {
    // se fallisce (es. fine giornata mentre aspetto):
    // => logga solo se non è EINTR pulito
    if (errno != EINTR) {
      LOG_ERR("UTENTE", "Nessuna risposta da %s", ROLE_NAME(type));
    }
    return -1;
  }

  if (resp.status == ORDER_SOLD_OUT) {
    LOG_WARN("UTENTE", "Operatore PID %d dice: Piatto Terminato!",
             resp.operator_pid);
    return -2;
  }

  if (type != OP_CASSA) {
    LOG_INFO("UTENTE", "Ricevuto un %s da Operatore %d.",
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
  LOG_INFO("UTENTE", "Cerco tavolo...");
  if (sem_wait(g_sem_id, SEM_INDEX_TABLES) == -1)
    return;

  LOG_INFO("UTENTE", "Mangio...");

  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = cfg->user_meal_duration_ns;

  nanosleep(&t, NULL);

  sem_signal(g_sem_id, SEM_INDEX_TABLES);
  LOG_INFO("UTENTE", "Pasto finito, libero tavolo.");
}

/**
 * @brief Loop giornaliero dell'utente.
 * * Ogni giorno l'utente "torna" in mensa.
 */
void user_routine(Config *cfg, double *current_budget) {
  double conto_da_pagare = 0.0;
  double budget_disponibile = *current_budget;

  LOG_INFO("UTENTE", "Patrimonio attuale: %.2f€", budget_disponibile);

  bool wants_primo = false;
  bool wants_secondo = false;
  bool wants_caffe = false;

  // prima il secondo
  if (random_probability(cfg->probability_user_wants_secondo, rand)) {
    if (budget_disponibile >= cfg->price_secondi) {
      wants_secondo = true;
      budget_disponibile -= cfg->price_secondi;
      conto_da_pagare += cfg->price_secondi;
    }
  }

  // poi il primo (per preferenza)
  if (random_probability(cfg->probability_user_wants_primo, rand)) {
    if (budget_disponibile >= cfg->price_primi) {
      wants_primo = true;
      budget_disponibile -= cfg->price_primi;
      conto_da_pagare += cfg->price_primi;
    }
  }

  if (random_probability(cfg->probability_user_wants_caffe, rand)) {
    if (budget_disponibile >= cfg->price_caffe) {
      wants_caffe = true;
      budget_disponibile -= cfg->price_caffe;
      conto_da_pagare += cfg->price_caffe;
    }
  }

  // fallback caffè
  if (!wants_primo && !wants_secondo && !wants_caffe &&
      budget_disponibile >= cfg->price_caffe) {
    wants_caffe = true;
    budget_disponibile -= cfg->price_caffe;
    conto_da_pagare += cfg->price_caffe;
  }

  if (conto_da_pagare == 0.0) {
    LOG_WARN("UTENTE", "Troppo povero oggi (%.2f€)! Salto il pasto.",
             *current_budget);
    return;
  }

  if (wants_primo) {
    if (enter_queue(SEM_INDEX_SEATS_PRIMI, "PRIMI",
                    cfg->user_queue_timeout_sec) == 0) {

      int outcome = perform_order(OP_PRIMI, MSG_TYPE_ORDER_PRIMI, 0.0);

      if (outcome == -1) {
        return;
      } else if (outcome == -2) {
        // caso piatti finiti: rimborso budget e non mangio
        *current_budget += cfg->price_primi;
        conto_da_pagare -= cfg->price_primi;
        LOG_INFO("UTENTE", "Niente primo oggi (esaurito). Risparmiati %.2f€",
                 (double)cfg->price_primi);
      }

      sem_signal(g_sem_id, SEM_INDEX_SEATS_PRIMI);
    } else {
      *current_budget += cfg->price_primi;
      conto_da_pagare -= cfg->price_primi;
    }
  }

  if (wants_secondo) {
    if (enter_queue(SEM_INDEX_SEATS_SECONDI, "SECONDI",
                    cfg->user_queue_timeout_sec) == 0) {
      int outcome = perform_order(OP_SECONDI, MSG_TYPE_ORDER_SECONDI, 0.0);

      if (outcome == -1) {
        return;
      } else if (outcome == -2) {
        // caso piatti finiti: rimborso budget e non mangio
        *current_budget += cfg->price_secondi;
        conto_da_pagare -= cfg->price_secondi;
        LOG_INFO("UTENTE", "Niente secondo oggi (esaurito). Risparmiati %.2f€",
                 (double)cfg->price_secondi);
      }

      sem_signal(g_sem_id, SEM_INDEX_SEATS_SECONDI);
    } else {
      *current_budget += cfg->price_secondi;
      conto_da_pagare -= cfg->price_secondi;
    }
  }

  if (wants_primo || wants_secondo) {
    consume_meal(cfg);
  }

  if (wants_caffe) {
    if (enter_queue(SEM_INDEX_SEATS_CAFFE, "CAFFE",
                    cfg->user_queue_timeout_sec) == 0) {
      int outcome = perform_order(OP_CAFFE, MSG_TYPE_ORDER_CAFFE, 0.0);

      if (outcome == -1) {
        return;
      } else if (outcome == -2) {
        // caso piatti finiti: rimborso budget e non mangio
        *current_budget += cfg->price_caffe;
        conto_da_pagare -= cfg->price_caffe;
        LOG_INFO("UTENTE", "Niente caffè oggi (esaurito). Risparmiati %.2f€",
                 (double)cfg->price_caffe);
      }

      sem_signal(g_sem_id, SEM_INDEX_SEATS_CAFFE);
    } else {
      *current_budget += cfg->price_caffe;
      conto_da_pagare -= cfg->price_caffe;
    }
  }

  // una consumazione è obligatoria, ma se il conto è 0 (tutto esaurito) esco
  if (conto_da_pagare <= 0.001) {
    LOG_INFO("UTENTE", "Non ho consumato nulla. Esco dalla mensa.");
    return;
  }

  if (enter_queue(SEM_INDEX_SEATS_CASSA, "CASSA",
                  cfg->user_queue_timeout_sec) == 0) {
    if (perform_order(OP_CASSA, MSG_TYPE_PAYMENT, conto_da_pagare) != -1) {
      *current_budget -= conto_da_pagare;
      LOG_INFO("UTENTE", "Pagamento di %.2f€ completato. Saldo residuo: %.2f€",
               conto_da_pagare, *current_budget);
    }
    sem_signal(g_sem_id, SEM_INDEX_SEATS_CASSA);
  }
}

int main(int argc, char *argv[]) {
  signal(SIGTERM, stop_handler);
  signal(SIGINT, stop_handler);
  signal(SIGUSR1, day_change_handler);

  srand((unsigned int)time(NULL) ^ (unsigned int)getpid());

  const char *config_path = (argc > 1) ? argv[1] : "conf/default.conf";
  Config config;
  if (parse_config(config_path, &config) == -1) {
    exit(EXIT_FAILURE);
  }

  if (setup_ipc() == -1) {
    exit(EXIT_FAILURE);
  }

  double my_budget =
      random_range(config.user_budget_min, config.user_budget_max, rand);

  LOG_INFO("UTENTE", "Cliente arrivato in mensa. Patrimonio iniziale: %.2f€",
           my_budget);

  for (int day = 1; day <= config.simulation_duration_days && g_running;
       day++) {
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

    LOG_INFO("UTENTE", "Giorno %d: Ricevuto stipendio %.2f€. Totale: %.2f€",
             day, daily_salary, my_budget);

    if (config.user_max_arrival_delay_us > 0) {
      struct timespec ts = {
          0,
          (long)random_range(0, config.user_max_arrival_delay_us, rand) * 1000};
      if (nanosleep(&ts, NULL) == -1 && !g_running) {
        break;
      }
    }

    user_routine(&config, &my_budget);

    LOG_INFO("UTENTE", "Finito il pasto, attendo chiusura mensa (Giorno %d)...",
             day);

    // attende il segnale SIGUSR1 dal Responsabile
    // pause() ritorna -1 con errno=EINTR quando arriva un segnale gestito
    if (g_running) {
      LOG_INFO("UTENTE",
               "Finito il pasto, attendo chiusura mensa (Giorno %d)...", day);
      pause();
    }

    // Quando arriva SIGUSR1, handle_day_end viene chiamato (vuoto o flag),
    // pause() si sblocca e il ciclo ricomincia.
  }

  return 0;
}