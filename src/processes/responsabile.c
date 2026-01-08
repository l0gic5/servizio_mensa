/**
 * @file responsabile.c
 * @brief Processo "Responsabile" (Manager).
 *
 * Questo file contiene il codice sorgente per il processo responsabile
 * che gestisce le risorse IPC e avvia i processi lavoratori e utenti.
 * - Crea e inizializza le risorse IPC (Shared Memory, Semafori, Code Messaggi)
 * - Legge il file di configurazione
 * - Avvia i processi Operatore e Cassa
 * - Avvia i processi Utente
 * - Gestisce la terminazione e il cleanup delle risorse
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "common/config.h"
#include "common/ipc_utils.h"
#include "common/logger.h"
#include "common/stats.h"
#include "common/types.h"

#define PATH_OPERATORE "./bin/processes/operatore"
#define PATH_CASSA "./bin/processes/cassa"
#define PATH_UTENTE "./bin/processes/utente"

static int g_sem_id = -1;
static int g_msg_id = -1;
static int g_shm_roles_id = -1;
static int g_shm_stats_id = -1;
static int g_shm_kitchen_id = -1;

static Config g_config;
static pid_t *g_child_pids = NULL;
static int g_total_children = 0;

static GlobalStats *g_stats = NULL;
static KitchenState *g_kitchen = NULL;
static WorkerConfig *g_worker_config = NULL;

static volatile sig_atomic_t g_shutdown = 0;

/**
 * @brief Funzione di pulizia risorse (chiamata a fine main o signal handler)
 */
void cleanup_resources(void) {
  LOG_CONF("RESPONSABILE", "Avvio procedura di cleanup (inviato SIGTERM)...");

  signal(SIGTERM, SIG_IGN);

  if (g_child_pids) {
    for (int i = 0; i < g_total_children; i++) {
      if (g_child_pids[i] > 0) {
        kill(g_child_pids[i], SIGTERM);
      }
    }

    sleep(1);

    for (int i = 0; i < g_total_children; i++) {
      if (g_child_pids[i] > 0) {
        int status;
        int res = waitpid(g_child_pids[i], &status, WNOHANG);

        if (res == 0) {
          kill(g_child_pids[i], SIGKILL);
        }
      }
    }

    while (wait(NULL) > 0) {
    }

    free(g_child_pids);
  }

  // rimozione risorse IPC
  if (g_sem_id != -1) {
    remove_sem_set(g_sem_id);
  }
  if (g_msg_id != -1) {
    remove_msg_queue(g_msg_id);
  }
  if (g_shm_stats_id != -1) {
    remove_shm(g_shm_stats_id);
  }
  if (g_shm_kitchen_id != -1) {
    remove_shm(g_shm_kitchen_id);
  }
  if (g_shm_roles_id != -1) {
    remove_shm(g_shm_roles_id);
  }

  // per utenti generati dinamicamente
  kill(0, SIGTERM);

  while (wait(NULL) > 0) {
  }

  LOG_CONF("RESPONSABILE", "Cleanup completato. Terminazione.");
}

/**
 * @brief Gestore segnali (SIGINT, SIGTERM)
 */
void signal_handler(int sig) {
  // EVITA IL warning: unused parameter ‘sig’ [-Wunused-parameter]
  (void)sig;

  LOG_WARN("RESPONSABILE", "Ricevuto segnale di interruzione. Chiusura...");

  g_shutdown = 1;
  // cleanup_resources();
  // exit(EXIT_SUCCESS);
}

/**
 * @brief Inizializza tutte le risorse IPC.
 * * @param out_stats Puntatore al puntatore stats del main (Output param).
 */
int setup_ipc() {
  // 1) SHM statistiche
  g_shm_stats_id = allocate_shm(sizeof(GlobalStats), FTOK_SHM_ID);
  if (g_shm_stats_id == -1) {
    return -1;
  }
  g_stats = (GlobalStats *)attach_shm(g_shm_stats_id);
  memset(g_stats, 0, sizeof(GlobalStats));

  g_shm_kitchen_id = allocate_shm(sizeof(KitchenState), FTOK_SHM_SUPPLY_ID);
  if (g_shm_kitchen_id == -1) {
    return -1;
  }
  g_kitchen = (KitchenState *)attach_shm(g_shm_kitchen_id);
  memset(g_kitchen, 0, sizeof(KitchenState));

  // 2) SHM ruoli operatori
  g_shm_roles_id = allocate_shm(sizeof(WorkerConfig), FTOK_SHM_ROLES_ID);
  if (g_shm_roles_id == -1) {
    return -1;
  }
  g_worker_config = (WorkerConfig *)attach_shm(g_shm_roles_id);
  memset(g_worker_config, 0, sizeof(WorkerConfig));

  // 3) coda messaggi
  g_msg_id = create_msg_queue();
  if (g_msg_id == -1) {
    return -1;
  }

  // 4) semafori
  g_sem_id = create_sem_set(TOTAL_SEMS);
  if (g_sem_id == -1) {
    return -1;
  }

  // Init Semafori Code Utenti
  init_sem(g_sem_id, SEM_INDEX_SEATS_PRIMI, g_config.queue_capacity_primi);
  init_sem(g_sem_id, SEM_INDEX_SEATS_SECONDI, g_config.queue_capacity_secondi);
  init_sem(g_sem_id, SEM_INDEX_SEATS_CAFFE, g_config.queue_capacity_caffe);
  init_sem(g_sem_id, SEM_INDEX_SEATS_CASSA, g_config.queue_capacity_cassa);
  init_sem(g_sem_id, SEM_INDEX_TICKET_READER, g_config.ticket_reader_capacity);
  init_sem(g_sem_id, SEM_INDEX_TABLES, g_config.nof_table_seats);

  // Init Mutex
  init_sem(g_sem_id, SEM_INDEX_MUTEX_STATS, 1);
  init_sem(g_sem_id, SEM_INDEX_OUTPUT, 1);
  init_sem(g_sem_id, SEM_INDEX_BARRIER, 0);
  init_sem(g_sem_id, SEM_INDEX_DAY_CHANGE, 0);

  // Init Semafori Workstations (Postazioni fisiche operatori)
  init_sem(g_sem_id, SEM_OPERATORS_PRIMI, g_config.workstations_primi);
  init_sem(g_sem_id, SEM_OPERATORS_SECONDI, g_config.workstations_secondi);
  init_sem(g_sem_id, SEM_OPERATORS_CAFFE, g_config.workstations_caffe);
  init_sem(g_sem_id, SEM_OPERATORS_CASSA, g_config.workstations_cassa);

  return 0;
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
 * @brief Calcola la distribuzione ottimale dei worker tra le stazioni.
 *
 * Utilizza un algoritmo "Greedy" (ingordo) per assegnare i worker disponibili.
 * Garantisce inizialmente almeno un worker per ogni stazione (Primi, Secondi,
 * Caffè). I worker rimanenti vengono assegnati iterativamente alla stazione che
 * presenta il carico di lavoro stimato (tempo di servizio / numero worker
 * attuali) più alto, riducendo così il collo di bottiglia del sistema.
 *
 * @param[in]  available_workers Numero totale di worker disponibili (esclusa la
 * cassa).
 * @param[in]  t_primi Tempo medio di servizio per i Primi (ns).
 * @param[in]  t_secondi Tempo medio di servizio per i Secondi (ns).
 * @param[in]  t_caffe Tempo medio di servizio per il Caffè (ns).
 * @param[out] w_primi Puntatore dove scrivere il numero di worker assegnati ai
 * Primi.
 * @param[out] w_secondi Puntatore dove scrivere il numero di worker assegnati
 * ai Secondi.
 * @param[out] w_caffe Puntatore dove scrivere il numero di worker assegnati al
 * Caffè.
 */
void compute_initial_workers_distribution(int available_workers, int t_primi,
                                          int t_secondi, int t_caffe,
                                          int *w_primi, int *w_secondi,
                                          int *w_caffe) {

  if (available_workers < 3) {
    LOG_ERR("RESPONSABILE", "Troppi pochi worker! Configurazione impossibile.");
    exit(EXIT_FAILURE);
  }

  // di default: 1 per tutti
  *w_primi = 1;
  *w_secondi = 1;
  *w_caffe = 1;

  int to_assign = available_workers - 3;

  while (to_assign > 0) {
    double load_primi = (double)t_primi / (*w_primi);
    double load_secondi = (double)t_secondi / (*w_secondi);
    double load_caffe = (double)t_caffe / (*w_caffe);

    if (load_primi >= load_secondi && load_primi >= load_caffe) {
      (*w_primi)++;
    } else if (load_secondi >= load_primi && load_secondi >= load_caffe) {
      (*w_secondi)++;
    } else {
      (*w_caffe)++;
    }
    to_assign--;
  }
}

/**
 * @brief Algoritmo Smart: Ricalcola i ruoli basandosi sullo stress reale.
 * * Invece di usare solo le stime del config, guarda i tempi di attesa medi
 * accumulati fino al giorno precedente. Assegna più risorse dove
 * l'attesa è maggiore.
 *
 * @param available_workers Numero totale di worker disponibili (esclusa la
 * cassa).
 */
void perform_dynamic_reconfiguration(int available_workers) {
  sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);

  double wait_p =
      (g_stats->total_plates_primi > 0)
          ? g_stats->total_wait_time_primi / g_stats->total_plates_primi
          : 0.0;

  double wait_s =
      (g_stats->total_plates_secondi > 0)
          ? g_stats->total_wait_time_secondi / g_stats->total_plates_secondi
          : 0.0;

  double wait_c =
      (g_stats->total_plates_caffe > 0)
          ? g_stats->total_wait_time_caffe / g_stats->total_plates_caffe
          : 0.0;

  sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);

  // assegnazione minima garantita (1 per tipo)
  int w_p = 1, w_s = 1, w_c = 1;
  int remaining = available_workers - 3;

  // fallback mancanza dati
  if (wait_p == 0 && wait_s == 0 && wait_c == 0) {
    wait_p = (double)g_config.avg_service_primi;
    wait_s = (double)g_config.avg_service_secondi;
    wait_c = (double)g_config.avg_service_caffe;
  }

  double total_wait = wait_p + wait_s + wait_c;

  if (total_wait > 0) {
    int extra_p = (int)((wait_p / total_wait) * remaining);
    int extra_s = (int)((wait_s / total_wait) * remaining);
    int extra_c = (int)((wait_c / total_wait) * remaining);

    w_p += extra_p;
    w_s += extra_s;
    w_c += extra_c;

    int assigned = extra_p + extra_s + extra_c;
    int leftovers = remaining - assigned;

    while (leftovers > 0) {
      if (wait_p >= wait_s && wait_p >= wait_c) {
        w_p++;
        wait_p /= 2;
      } else if (wait_s >= wait_p && wait_s >= wait_c) {
        w_s++;
        wait_s /= 2;
      } else {
        w_c++;
        wait_c /= 2;
      }
      leftovers--;
    }
  } else {
    while (remaining > 0) {
      if (remaining > 0) {
        w_p++;
        remaining--;
      }
      if (remaining > 0) {
        w_s++;
        remaining--;
      }
      if (remaining > 0) {
        w_c++;
        remaining--;
      }
    }
  }

  sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);

  int active_cassa = g_worker_config->active_cassa;

  int index = 1;

  for (int k = 0; k < w_p; k++) {
    g_worker_config->worker_roles[index++] = OP_PRIMI;
  }
  for (int k = 0; k < w_s; k++) {
    g_worker_config->worker_roles[index++] = OP_SECONDI;
  }
  for (int k = 0; k < w_c; k++) {
    g_worker_config->worker_roles[index++] = OP_CAFFE;
  }

  g_worker_config->active_primi = w_p;
  g_worker_config->active_secondi = w_s;
  g_worker_config->active_caffe = w_c;

  sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);

  LOG_CONF(
      "RESPONSABILE",
      "Reconfig Smart (basata su attese): Cassa: %d, Primi:%d, Secondi:%d, "
      "Caffè:%d",
      active_cassa, w_p, w_s, w_c);
}

/**
 * @brief Helper per spawnare un gruppo di worker dello stesso tipo.
 */
void spawn_worker_group(OpType role, int count, const char *path,
                        const char *config_path, int *current_id,
                        int *pid_index) {
  char id_str[10];

  for (int i = 0; i < count; i++) {
    g_worker_config->worker_roles[*current_id] = role;

    sprintf(id_str, "%d", *current_id);
    char *args[] = {(char *)path, id_str, (char *)config_path, NULL};

    g_child_pids[(*pid_index)++] = spawn_process(path, args);

    (*current_id)++;
  }
}

/**
 * @brief Avvia tutti i processi (Workers e Utenti) con la distribuzione
 * iniziale.
 */
void start_all_processes(const char *config_path) {
  g_total_children = g_config.nof_workers + g_config.nof_users;
  g_child_pids = malloc(sizeof(pid_t) * (size_t)g_total_children);

  if (!g_child_pids) {
    LOG_ERR("RESPONSABILE", "Malloc fallita");
    exit(EXIT_FAILURE);
  }

  int pid_index = 0;
  int current_worker_id = 0;

  int w_cassa = 1;
  int w_primi, w_secondi, w_caffe;

  compute_initial_workers_distribution(
      g_config.nof_workers - w_cassa, g_config.avg_service_primi,
      g_config.avg_service_secondi, g_config.avg_service_caffe, &w_primi,
      &w_secondi, &w_caffe);

  sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);

  g_worker_config->active_primi = w_primi;
  g_worker_config->active_secondi = w_secondi;
  g_worker_config->active_caffe = w_caffe;
  g_worker_config->active_cassa = w_cassa;

  sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);

  LOG_CONF("RESPONSABILE",
           "Distribuzione Iniziale:\n  - Cassa: %d\n  - Primi: %d\n  - "
           "Secondi: %d\n  - Caffè: %d",
           w_cassa, w_primi, w_secondi, w_caffe);

  // spawn Workers
  spawn_worker_group(OP_CASSA, w_cassa, PATH_CASSA, config_path,
                     &current_worker_id, &pid_index);
  spawn_worker_group(OP_PRIMI, w_primi, PATH_OPERATORE, config_path,
                     &current_worker_id, &pid_index);
  spawn_worker_group(OP_SECONDI, w_secondi, PATH_OPERATORE, config_path,
                     &current_worker_id, &pid_index);
  spawn_worker_group(OP_CAFFE, w_caffe, PATH_OPERATORE, config_path,
                     &current_worker_id, &pid_index);

  // spawn Utenti
  for (int i = 0; i < g_config.nof_users; i++) {
    int has_ticket = random_probability(g_config.avg_user_w_ticket, rand);

    char ticket_arg[2];
    sprintf(ticket_arg, "%d", has_ticket);

    char *args_utente[] = {(char *)PATH_UTENTE, (char *)config_path, ticket_arg,
                           NULL};

    g_child_pids[pid_index++] = spawn_process(PATH_UTENTE, args_utente);
  }

  sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);
  g_worker_config->total_workers_count = g_config.nof_workers;
  g_worker_config->active_primi = w_primi;
  sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);

  sleep((unsigned int)g_config.system_startup_delay_sec);

  LOG_INFO("RESPONSABILE", "Processi avviati: %d", pid_index);
}

/**
 * @brief Gestisce stampa e export del report giornaliero calcolato.
 */
void handle_daily_stats(DailyReport *report, GlobalStats *total_stats) {
  sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);

  sem_mutex_acquire(g_sem_id, SEM_INDEX_OUTPUT);
  char *daily_log = process_daily_report(report, total_stats);
  if (daily_log) {
    printf("%s", daily_log);
    free(daily_log);
  }

  if (g_config.export_daily_reports_csv) {
    const char *final_csv_name = g_config.export_final_stats_csv
                                     ? g_config.final_stats_filename_csv
                                     : NULL;

    export_daily_stats_to_csv(report, g_config.export_folder_path,
                              g_config.daily_reports_filename_csv,
                              final_csv_name,
                              g_config.create_daily_single_files);
  }

  sem_mutex_release(g_sem_id, SEM_INDEX_OUTPUT);
  sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);
}

/**
 * @brief Gestisce stampa e export delle statistiche finali
 */
void handle_final_stats(int days_completed) {
  sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);
  sem_mutex_acquire(g_sem_id, SEM_INDEX_OUTPUT);

  char *final_log = process_final_report(g_stats, days_completed);
  if (final_log) {
    printf("%s", final_log);
    free(final_log);
  }

  if (g_config.export_final_stats_csv) {
    export_final_stats_to_csv(g_stats, days_completed,
                              g_config.export_folder_path,
                              g_config.final_stats_filename_csv);
  }

  sem_mutex_release(g_sem_id, SEM_INDEX_OUTPUT);
  sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);
}

/**
 * @brief Esegue il rifornimento periodico delle stazioni (ogni 10 min).
 * Aggiunge porzioni fino al raggiungimento della capacità massima.
 */
void perform_periodic_refill() {
  sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);

  bool refilled = false;

  // rifornimento PRIMI
  int current_primi = g_kitchen->remaining_primi;
  int to_add_primi = g_config.avg_refill_primi;
  if (current_primi < g_config.max_porzioni_primi) {
    int new_quantity = current_primi + to_add_primi;

    if (new_quantity > g_config.max_porzioni_primi) {
      new_quantity = g_config.max_porzioni_primi;
    }

    g_kitchen->remaining_primi = new_quantity;
    g_stats->total_refilled_primi += new_quantity - current_primi;

    refilled = true;
  }

  // rifornimento SECONDI
  int current_secondi = g_kitchen->remaining_secondi;
  int to_add_secondi = g_config.avg_refill_secondi;
  if (current_secondi < g_config.max_porzioni_secondi) {
    int new_quantity = current_secondi + to_add_secondi;

    if (new_quantity > g_config.max_porzioni_secondi) {
      new_quantity = g_config.max_porzioni_secondi;
    }

    g_kitchen->remaining_secondi = new_quantity;
    g_stats->total_refilled_secondi += new_quantity - current_secondi;
    refilled = true;
  }

  // rifornimento CAFFÈ => (max_porzioni_caffe - remaining_caffe) == caffè
  // infinito!
  int current_caffe = g_kitchen->remaining_caffe;
  int to_add_caffe = g_config.max_porzioni_caffe - g_kitchen->remaining_caffe;
  if (current_caffe < g_config.max_porzioni_caffe) {
    int new_quantity = current_caffe + to_add_caffe;

    if (new_quantity > g_config.max_porzioni_caffe) {
      new_quantity = g_config.max_porzioni_caffe;
    }

    g_kitchen->remaining_caffe = new_quantity;
    g_stats->total_refilled_caffe += new_quantity - current_caffe;
    refilled = true;
  }

  if (refilled) {
    LOG_INFO("RESPONSABILE", "Refill periodico (%d min) eseguito.",
             g_config.refill_interval_minutes);
  }
  // else {
  //   LOG_INFO("RESPONSABILE", "Refill non necessario (cucina piena).");
  // }

  // LOG_INFO("RESPONSABILE",
  //          "Refill eseguito. Stato cucina:\n  - %d Primi\n  - %d Secondi\n  -
  //          "
  //          "%d Caffè",
  //          g_kitchen->remaining_primi, g_kitchen->remaining_secondi,
  //          g_kitchen->remaining_caffe);
  sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);
}

/**
 * @brief Esegue il ciclo temporale della giornata (simulazione ore lavorative).
 */
void simulate_day_cycle() {
  int total_minutes = (g_config.daily_service_minutes > 0)
                          ? g_config.daily_service_minutes
                          : 120;
  int num_cycles = total_minutes / g_config.refill_interval_minutes;
  int remainder_minutes = total_minutes % g_config.refill_interval_minutes;

  // calcolo nanosecondi per ciclo di refill
  long cycle_nanos =
      (long)g_config.n_nanosecs_as_minute * g_config.refill_interval_minutes;
  struct timespec ts_cycle = {0, 0};
  ts_cycle.tv_sec = cycle_nanos / 1000000000L;
  ts_cycle.tv_nsec = cycle_nanos % 1000000000L;

  for (int i = 0; i < num_cycles; i++) {
    if (g_shutdown) {
      return;
    }

    if (nanosleep(&ts_cycle, NULL) == -1 && errno == EINTR) {
      if (g_shutdown) {
        return;
      }
    }
    perform_periodic_refill();
  }

  // minuti residui
  if (!g_shutdown && remainder_minutes > 0) {
    long rem_nanos = (long)g_config.n_nanosecs_as_minute * remainder_minutes;
    struct timespec ts_rem = {0, 0};
    ts_rem.tv_sec = rem_nanos / 1000000000L;
    ts_rem.tv_nsec = rem_nanos % 1000000000L;
    nanosleep(&ts_rem, NULL);
  }
}

/**
 * @brief Gestisce la fine della giornata: invia segnali e attende barriera.
 * Implementa logica robusta anti-deadlock (Zombie Reaping).
 */
void handle_day_end_sync() {
  int active_children = 0;

  // invia SIGUSR1 solo ai processi vivi
  for (int i = 0; i < g_total_children; i++) {
    if (g_child_pids[i] > 0) {
      int status;
      pid_t res = waitpid(g_child_pids[i], &status, WNOHANG);

      if (res == 0) {
        // processo vivo
        if (kill(g_child_pids[i], SIGUSR1) == 0) {
          active_children++;
        } else {
          g_child_pids[i] = 0;
        }
      } else {
        // processo morto (zombie o errore)
        if (res > 0) {
          LOG_WARN("RESPONSABILE", "PID %d terminato prematuramente.",
                   g_child_pids[i]);
        }
        g_child_pids[i] = 0;
      }
    }
  }

  LOG_CONF("RESPONSABILE", "Attesa sincronizzazione da %d processi...",
           active_children);

  // barrier wait (sincronizzazione fine giornata)
  for (int i = 0; i < active_children; i++) {
    struct sembuf sb = {SEM_INDEX_BARRIER, -1, 0};
    struct timespec timeout = {g_config.day_end_barrier_wait_sec, 0};

    if (semtimedop(g_sem_id, &sb, 1, &timeout) == -1) {

      // CASO 1 => interruzione da segnale
      if (errno == EINTR) {
        // decremento 'i' => prossimo ciclo del for fa i++
        i--;

        if (g_shutdown) {
          break;
        }
      }
      // CASO 2 => errore (timeout o errore critico)
      else {
        if (errno == EAGAIN) {
          LOG_ERR("RESPONSABILE",
                  "Timeout barriera fine giornata: il processo %d "
                  "non ha sincronizzato. Forzo shutdown.",
                  g_child_pids[i]);
        } else {
          LOG_ERR("RESPONSABILE", "Errore critico wait barriera");
        }

        g_shutdown = 1;

        // best effort: termina i figli rimasti per evitare blocchi su IPC
        for (int k = 0; k < g_total_children; k++) {
          if (g_child_pids && g_child_pids[k] > 0) {
            kill(g_child_pids[k], SIGTERM);
          }
        }
        break;
      }
    }
  }

  LOG_CONF("RESPONSABILE", "Sincronizzazione completata.");
}

/**
 * @brief Loop principale della simulazione.
 */
void run_simulation_loop(const char *config_path, int *day) {
  bool overload = false;
  GlobalStats start_of_day_stats = {0};

  for ((*day) = 1; (*day) <= g_config.simulation_duration_days && !overload;
       (*day)++) {
    LOG_INFO("RESPONSABILE", COLOR_CYAN "Inizio Giorno %d" COLOR_RESET, (*day));

    // avvio o refill giornaliero
    if ((*day) == 1) {
      start_all_processes(config_path);
    } else {
      sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);
      g_kitchen->remaining_primi = g_config.max_porzioni_primi;
      g_kitchen->remaining_secondi = g_config.max_porzioni_secondi;
      g_kitchen->remaining_caffe = g_config.max_porzioni_caffe;
      // snapshot inizio giornata
      start_of_day_stats = *g_stats;

      g_worker_config->current_day = *day;
      sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);

      // segnalo inizio giornata
      struct sembuf sb = {SEM_INDEX_DAY_CHANGE, 1, 0};
      semop(g_sem_id, &sb, 1);

      LOG_CONF("RESPONSABILE", "Cucina rifornita (Day Start).");
    }

    if ((*day) > 1) {
      perform_dynamic_reconfiguration(g_config.nof_workers - 1);
    }

    // simulazione tempo
    simulate_day_cycle();

    if (g_shutdown) {
      LOG_WARN("RESPONSABILE", "Interruzione rilevata. Ripristino statistiche "
                               "all'ultimo giorno completo.");
      sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);
      *g_stats = start_of_day_stats;
      sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);

      break;
    }

    // fine giornata & sincronizzazione
    handle_day_end_sync();

    if (g_shutdown) {
      LOG_WARN("RESPONSABILE",
               "Interruzione durante sync. Ripristino statistiche.");
      sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);
      *g_stats = start_of_day_stats;
      sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);
      break;
    }

    DailyReport report;
    memset(&report, 0, sizeof(DailyReport));
    report.day_number = (*day);

    sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);
    GlobalStats end = *g_stats;
    KitchenState leftovers = *g_kitchen;
    sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);

    // calcolo delta per DailyReport
    report.daily_users_served =
        end.total_users_served - start_of_day_stats.total_users_served;
    report.daily_users_w_ticket =
        end.total_users_w_ticket - start_of_day_stats.total_users_w_ticket;
    report.daily_users_refused =
        end.total_users_refused - start_of_day_stats.total_users_refused;
    report.daily_plates_primi =
        end.total_plates_primi - start_of_day_stats.total_plates_primi;
    report.daily_plates_secondi =
        end.total_plates_secondi - start_of_day_stats.total_plates_secondi;
    report.daily_plates_caffe =
        end.total_plates_caffe - start_of_day_stats.total_plates_caffe;
    report.daily_revenue = end.total_revenue - start_of_day_stats.total_revenue;

    report.daily_transactions =
        end.total_transactions - start_of_day_stats.total_transactions;

    report.leftover_primi =
        (leftovers.remaining_primi > 0) ? leftovers.remaining_primi : 0;
    report.leftover_secondi =
        (leftovers.remaining_secondi > 0) ? leftovers.remaining_secondi : 0;
    report.leftover_caffe =
        (leftovers.remaining_caffe > 0) ? leftovers.remaining_caffe : 0;

    report.daily_refilled_primi =
        end.total_refilled_primi - start_of_day_stats.total_refilled_primi;
    report.daily_refilled_secondi =
        end.total_refilled_secondi - start_of_day_stats.total_refilled_secondi;
    report.daily_refilled_caffe =
        end.total_refilled_caffe - start_of_day_stats.total_refilled_caffe;

    // delta tempi
    report.daily_wait_primi =
        end.total_wait_time_primi - start_of_day_stats.total_wait_time_primi;
    report.daily_wait_secondi = end.total_wait_time_secondi -
                                start_of_day_stats.total_wait_time_secondi;
    report.daily_wait_caffe =
        end.total_wait_time_caffe - start_of_day_stats.total_wait_time_caffe;
    report.daily_wait_cassa =
        end.total_wait_time_cassa - start_of_day_stats.total_wait_time_cassa;

    sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);
    g_stats->total_leftover_primi += report.leftover_primi;
    g_stats->total_leftover_secondi += report.leftover_secondi;
    g_stats->total_leftover_caffe += report.leftover_caffe;
    sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);

    handle_daily_stats(&report, g_stats);

    // aggiorno snapshot per domani
    start_of_day_stats = end;

    // check overload
    if (report.daily_users_refused > g_config.overload_threshold) {
      LOG_ERR("RESPONSABILE", "TERMINAZIONE: Overload (%d > %d)",
              report.daily_users_refused, g_config.overload_threshold);
      overload = true;
    }
  }

  if (!overload && !g_shutdown) {
    LOG_INFO("RESPONSABILE", "Simulazione completata.");
  }
}

int main(int argc, char *argv[]) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  const char *config_path = (argc > 1) ? argv[1] : "conf/default.conf";
  if (parse_config(config_path, &g_config) == -1) {
    LOG_ERR("RESPONSABILE", "Errore parsing config: %s", config_path);
    exit(EXIT_FAILURE);
  }

  if (g_config.n_nanosecs_as_minute <= 0) {
    g_config.n_nanosecs_as_minute = 1000000;
  }

  LOG_CONF("RESPONSABILE", "Configurazione `%s` caricata. Durata: %d gg",
           config_path, g_config.simulation_duration_days);

  if (setup_ipc(&g_stats) == -1) {
    LOG_ERR("RESPONSABILE", "Errore setup IPC");
    cleanup_resources();
    exit(EXIT_FAILURE);
  }

  // MUTEX non necessario perché per ora non è memoria competitiva
  // sem_mutex_acquire(g_sem_id, SEM_INDEX_MUTEX_STATS);

  g_kitchen->remaining_primi = g_config.max_porzioni_primi;
  g_kitchen->remaining_secondi = g_config.max_porzioni_secondi;
  g_kitchen->remaining_caffe = g_config.max_porzioni_caffe;

  // sem_mutex_release(g_sem_id, SEM_INDEX_MUTEX_STATS);

  LOG_CONF(
      "RESPONSABILE",
      "Rifornimento iniziale completato:\n  - %d Primi\n  - %d Secondi\n  - "
      "%d Caffè",
      g_config.max_porzioni_primi, g_config.max_porzioni_secondi,
      g_config.max_porzioni_caffe);

  int day_counter = 1;

  run_simulation_loop(config_path, &day_counter);

  handle_final_stats(day_counter - 1);
  cleanup_resources();

  return 0;
}
