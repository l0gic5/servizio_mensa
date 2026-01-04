#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "common/config.h"
#include "common/ipc_utils.h"
#include "common/logger.h"
#include "common/types.h"

static int g_shm_id = -1;
static int g_sem_id = -1;
static int g_msg_id = -1;
static Config g_config;
static pid_t *g_child_pids = NULL;
static int g_total_children = 0;

#define PATH_OPERATORE "./bin/operatore"
#define PATH_CASSA "./bin/cassa"
#define PATH_UTENTE "./bin/utente"

#define MINUTI_SERVIZIO_GIORNALIERO 120

/**
 * @brief Funzione di pulizia risorse (chiamata a fine main o signal handler)
 */
void cleanup_resources(void) {
  LOG_INFO("RESPONSABILE", "Avvio procedura di cleanup...");

  // SIGTERM a tutti i figli
  if (g_child_pids) {
    for (int i = 0; i < g_total_children; i++) {
      if (g_child_pids[i] > 0) {
        kill(g_child_pids[i], SIGTERM);
      }
    }

    // evita zombie
    while (wait(NULL) > 0) {
    }

    free(g_child_pids);
  }

  // risorse IPC
  if (g_shm_id != -1)
    remove_shm(g_shm_id);
  if (g_sem_id != -1)
    remove_sem_set(g_sem_id);
  if (g_msg_id != -1)
    remove_msg_queue(g_msg_id);

  LOG_INFO("RESPONSABILE", "Cleanup completato. Terminazione.");
}

/**
 * @brief Gestore segnali (SIGINT, SIGTERM)
 */
void signal_handler(int sig) {
  // EVITA IL warning: unused parameter ‘sig’ [-Wunused-parameter]
  (void)sig;

  LOG_WARN("RESPONSABILE", "Ricevuto segnale di interruzione. Chiusura...");
  cleanup_resources();
  exit(EXIT_SUCCESS);
}

/**
 * @brief Helper per lanciare un processo con execve
 */
pid_t spawn_process(const char *path, char *const argv[]) {
  pid_t pid = fork();

  if (pid == -1) {
    TEST_ERROR;
    return -1;
  }

  if (pid == 0) {
    // figlio
    execve(path, argv, NULL);

    // se execve ha fallito
    TEST_ERROR;
    exit(EXIT_FAILURE);
  }
  return pid;
}

/**
 * @brief Stampa le statistiche giornaliere
 */
void print_daily_stats(int day, Statistics *stats) {
  // protezione lettura statistiche
  sem_wait(g_sem_id, SEM_INDEX_MUTEX_STATS);
  sem_wait(g_sem_id, SEM_INDEX_OUTPUT);

  printf("\n" COLOR_BLUE "======== REPORT GIORNO %d =========" COLOR_RESET "\n",
         day);
  printf("Utenti Serviti Totali: %d\n", stats->total_users_served);
  printf("Utenti Respinti/Overload: %d\n", stats->total_users_refused);
  printf("Piatti Distribuiti:\n  - Primi: %d\n  - Secondi: %d\n  - Caffè: %d\n",
         stats->plates_primi, stats->plates_secondi, stats->plates_coffee);
  printf("Piatti Avanzati:\n  - Primi: %d\n  - Secondi: %d\n  - Caffè: %d\n",
         stats->leftover_primi, stats->leftover_secondi,
         stats->leftover_coffee);
  printf("Ricavo Totale: %.2f€\n", stats->total_revenue);
  printf(COLOR_BLUE "=================================" COLOR_RESET "\n\n");

  sem_signal(g_sem_id, SEM_INDEX_OUTPUT);
  sem_signal(g_sem_id, SEM_INDEX_MUTEX_STATS);
}

int main(int argc, char *argv[]) {
  // segnali per cleanup pulito
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  const char *config_path = (argc > 1) ? argv[1] : "conf/default.conf";

  if (parse_config(config_path, &g_config) == -1) {
    LOG_ERR("RESPONSABILE", "Errore parsing config: %s", config_path);
    exit(EXIT_FAILURE);
  }

  if (g_config.n_nano_secs <= 0) {
    LOG_WARN("RESPONSABILE", "N_NANO_SECS non valido, uso default 1ms");
    g_config.n_nano_secs = 1000000;
  }

  LOG_INFO("RESPONSABILE", "Configurazione caricata\n  - Durata: %d gg\n  - Utenti: %d",
           g_config.sim_duration, g_config.n_users);

  // risorse IPC

  // shared memory (statistiche)
  g_shm_id = allocate_shm(sizeof(Statistics));
  if (g_shm_id == -1)
    exit(EXIT_FAILURE);

  Statistics *stats = (Statistics *)attach_shm(g_shm_id);
  memset(stats, 0, sizeof(Statistics));

  // semafori
  g_sem_id = create_sem_set(TOTAL_SEMS);
  if (g_sem_id == -1) {
    cleanup_resources();
    exit(EXIT_FAILURE);
  }

  // init semafori, posti e code
  init_sem(g_sem_id, SEM_INDEX_SEATS_PRIMI, g_config.seats_primi);
  init_sem(g_sem_id, SEM_INDEX_SEATS_SECONDI, g_config.seats_secondi);
  init_sem(g_sem_id, SEM_INDEX_SEATS_COFFEE, g_config.seats_coffee);
  init_sem(g_sem_id, SEM_INDEX_SEATS_CASSA, g_config.seats_cassa);
  init_sem(g_sem_id, SEM_INDEX_TABLES, g_config.table_seats); // [cite: 84]

  // init MUTual EXclusion (1 = libero)
  // (impedisce che due processi tocchino la stessa risorsa contemporaneamente)
  init_sem(g_sem_id, SEM_INDEX_MUTEX_STATS, 1);
  init_sem(g_sem_id, SEM_INDEX_OUTPUT, 1);

  // message queue
  g_msg_id = create_msg_queue();
  if (g_msg_id == -1) {
    cleanup_resources();
    exit(EXIT_FAILURE);
  }

  // processi figli

  // tracciamento dei PID
  g_total_children = g_config.n_workers + g_config.n_users;
  g_child_pids = malloc(sizeof(pid_t) * (size_t)g_total_children);
  int pid_idx = 0;

  // === OPERATORI ===
  // distribuzione dei workers tra le stazioni

  int w_cassa = 1;

  int available_workers = g_config.n_workers - w_cassa;

  if (available_workers < 3) {
    LOG_ERR("RESPONSABILE", "Troppi pochi worker! Configurazione impossibile.");
    exit(EXIT_FAILURE);
  }

  // di default: 1 per tutti
  int w_primi = 1;
  int w_secondi = 1;
  int w_coffee = 1;

  available_workers -= 3;

  while (available_workers > 0) {
    double load_primi = (double)g_config.avg_service_primi / w_primi;
    double load_secondi = (double)g_config.avg_service_secondi / w_secondi;
    double load_coffee = (double)g_config.avg_service_coffee / w_coffee;

    if (load_primi >= load_secondi && load_primi >= load_coffee) {
      w_primi++;
    } else if (load_secondi >= load_primi && load_secondi >= load_coffee) {
      w_secondi++;
    } else {
      w_coffee++;
    }

    available_workers--;
  }

  LOG_INFO(
      "RESPONSABILE",
      "Distribuzione Workers (basata sui tempi)\n  - Cassa: %d\n  - Primi: %d "
      "(t=%d)\n  - Secondi: %d (t=%d)\n  - Coffee: %d (t=%d)",
      w_cassa, w_primi, g_config.avg_service_primi, w_secondi,
      g_config.avg_service_secondi, w_coffee, g_config.avg_service_coffee);

  // SPAWN cassa
  char *args_cassa[] = {PATH_CASSA, (char *)config_path, NULL};
  for (int i = 0; i < w_cassa; i++) {
    g_child_pids[pid_idx++] = spawn_process(PATH_CASSA, args_cassa);
  }

  // SPAWN operatori
  char *args_primi[] = {PATH_OPERATORE, "PRIMI", (char *)config_path, NULL};
  for (int i = 0; i < w_primi; i++) {
    g_child_pids[pid_idx++] = spawn_process(PATH_OPERATORE, args_primi);
  }

  char *args_secondi[] = {PATH_OPERATORE, "SECONDI", (char *)config_path, NULL};
  for (int i = 0; i < w_secondi; i++) {
    g_child_pids[pid_idx++] = spawn_process(PATH_OPERATORE, args_secondi);
  }

  char *args_coffee[] = {PATH_OPERATORE, "COFFEE", (char *)config_path, NULL};
  for (int i = 0; i < w_coffee; i++) {
    g_child_pids[pid_idx++] = spawn_process(PATH_OPERATORE, args_coffee);
  }

  // === UTENTI ===
  char *args_utente[] = {PATH_UTENTE, (char *)config_path, NULL};
  for (int i = 0; i < g_config.n_users; i++) {
    g_child_pids[pid_idx++] = spawn_process(PATH_UTENTE, args_utente);
  }

  sleep(1);
  LOG_INFO("RESPONSABILE", "Tutti i processi avviati. INIZIO SIMULAZIONE.");

  ////////////////////////////////////
  //  =!=  SIMULAZIONE GIORNI  =!=  //
  ////////////////////////////////////

  bool overload = false;
  for (int day = 1; day <= g_config.sim_duration && !overload; day++) {
    LOG_INFO("RESPONSABILE", COLOR_CYAN "Inizio Giorno %d" COLOR_RESET, day);

    // scorrere del tempo
    // N_NANO_SECS * MINUTI_SERVIZIO_GIORNALIERO
    struct timespec ts;
    long total_nanos = (long)g_config.n_nano_secs * MINUTI_SERVIZIO_GIORNALIERO;

    ts.tv_sec = total_nanos / 1000000000L;
    ts.tv_nsec = total_nanos % 1000000000L;

    nanosleep(&ts, NULL);

    // FINE GIORNATA => report e controlli
    print_daily_stats(day, stats);

    // controllo terminazione: OVERLOAD?
    if (stats->total_users_refused > g_config.overload_threshold) {
      LOG_ERR("RESPONSABILE",
              "TERMINAZIONE ANTICIPATA: Overload rilevato (%d > %d)",
              stats->total_users_refused, g_config.overload_threshold);

      overload = true;
    }

    // Reset statistiche giornaliere se necessario (il PDF chiede statistiche
    // "nella giornata") Qui dovremmo resettare i contatori parziali se la
    // struct lo supportasse, ma la struct attuale accumula i totali. Opzionale:
    // implementare reset "daily_XXX" in futuro.
  }

  LOG_INFO("RESPONSABILE", "Simulazione Terminata (Timeout o Overload).");

  print_daily_stats(g_config.sim_duration, stats);

  cleanup_resources();
  return 0;
}