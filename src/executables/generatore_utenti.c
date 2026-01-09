/**
 * @file generatore_utenti.c
 * @brief Tool per la Generazione Dinamica di Utenti (Smart Lifecycle).
 *
 * Spawna utenti e attende che il Responsabile dichiari la fine del giorno
 * prima di terminarli.
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "common/config.h"
#include "common/ipc_utils.h"
#include "common/logger.h"
#include "common/types.h"

#define PATH_UTENTE "./bin/processes/utente"

static volatile int keep_running = 1;

void int_handler(int sig) {
  (void)sig;
  keep_running = 0;
}

pid_t spawn_user(const char *config_path, Config *cfg) {
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    return -1;
  }
  if (pid == 0) {
    char *ticket_arg = (rand() % 100 < cfg->avg_user_w_ticket) ? "1" : "0";

    char *group_id_arg = "0";
    char *group_size_arg = "1";

    char *args[] = {(char *)PATH_UTENTE, (char *)config_path, ticket_arg,
                    group_id_arg,        group_size_arg,      NULL};

    execve(PATH_UTENTE, args, NULL);
    perror("execve");
    exit(EXIT_FAILURE);
  }
  return pid;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Uso: %s <path_config> [numero_utenti]\n", argv[0]);
    return EXIT_FAILURE;
  }

  signal(SIGINT, int_handler);
  srand((unsigned int)time(NULL) ^ (unsigned int)getpid());

  const char *config_path =
      (argv[1][0] != '\0') ? argv[1] : "conf/default.conf";
  int num_users_to_spawn = 0;

  if (argc >= 3) {
    num_users_to_spawn = atoi(argv[2]);
  }

  int shm_id = allocate_shm(sizeof(WorkerConfig), FTOK_SHM_ROLES_ID);
  if (shm_id == -1) {
    printf(COLOR_RED "Errore: Impossibile leggere SHM. Simulazione non "
                     "avviata?\n" COLOR_RESET);
    return EXIT_FAILURE;
  }
  WorkerConfig *worker_config = (WorkerConfig *)attach_shm(shm_id);

  Config config;
  if (parse_config(config_path, &config) == -1) {
    printf(
        COLOR_RED
        "Errore: Impossibile leggere file di configurazione: %s\n" COLOR_RESET,
        config_path);
    detach_shm(worker_config);
    return EXIT_FAILURE;
  }

  printf("\033[H\033[J");
  printf("\n" COLOR_CYAN
         "=== GENERATORE UTENTI DINAMICO (Smart Sync) ===" COLOR_RESET "\n");

  if (num_users_to_spawn <= 0) {
    printf(" * Inserisci numero utenti da generare: ");
    if (scanf("%d", &num_users_to_spawn) != 1 || num_users_to_spawn <= 0) {
      detach_shm(worker_config);
      return EXIT_FAILURE;
    }
  }

  int start_day = worker_config->current_day;
  printf(" ➯ Giorno Attuale rilevato: %d\n", start_day);
  printf(" ➯ Gli utenti vivranno finché non inizia il Giorno %d.\n",
         start_day + 1);

  pid_t *pids = malloc(sizeof(pid_t) * (size_t)num_users_to_spawn);

  printf(COLOR_YELLOW "\nSPAWN DI %d UTENTI..." COLOR_RESET "\n\n",
         num_users_to_spawn);

  struct shmid_ds buf;
  if (shmctl(shm_id, IPC_STAT, &buf) == -1) {
    printf(COLOR_RED "Errore: La Shared Memory non esiste più. Responsabile "
                     "morto?\n" COLOR_RESET);
    detach_shm(worker_config);
    return EXIT_FAILURE;
  }

  for (int i = 0; i < num_users_to_spawn; i++) {
    pids[i] = spawn_user(config_path, &config);
    if (pids[i] > 0) {
      printf(" ➤ Spawn di PID: %d\n", pids[i]);
      usleep(20000); // 20ms delay
    }
  }

  printf("----------------------------------------------------------\n");
  printf("Utenti attivi. In attesa del cambio giorno del Responsabile...\n");

  int sem_id = create_sem_set(TOTAL_SEMS);

  while (1) {
    struct sembuf sb_drain = {SEM_INDEX_DAY_CHANGE, -1, IPC_NOWAIT};
    if (semop(sem_id, &sb_drain, 1) == -1) {
      if (errno == EAGAIN) {
        break;
      }
      break;
    }
  }

  struct sembuf sb = {SEM_INDEX_DAY_CHANGE, -1, 0};

  if (semop(sem_id, &sb, 1) == -1) {
    if (errno == EINTR) {
      printf("\n[INFO] Interrotto dall'utente (CTRL+C locale).\n");
    } else if (errno == EIDRM || errno == EINVAL) {
      printf(COLOR_RED "\n[ALLERT] Il Responsabile è terminato (Semaforo "
                       "rimosso)!\n" COLOR_RESET);
    } else {
      perror("semop wait day change");
    }
    keep_running = 0;
  } else {
    printf(COLOR_PURPLE "\n[EVENTO] Segnale ricevuto! Il Responsabile ha "
                        "cambiato giorno (%d)!\n" COLOR_RESET,
           worker_config->current_day);
  }

  if (keep_running) {
    printf("Attendo 5 secondi di tolleranza per chi sta finendo...\n");
    for (int i = 5; i > 0; i--) {
      printf("\rChiusura forzata tra: %d s... ", i);
      fflush(stdout);
      sleep(1);
    }
    printf("\n");
  }

  printf(COLOR_RED
         "Tempo scaduto.\nTerminazione utenti ancora vivi...\n" COLOR_RESET);

  for (int i = 0; i < num_users_to_spawn; i++) {
    if (pids[i] > 0) {
      kill(pids[i], SIGTERM);
    }
  }

  // zombie
  for (int i = 0; i < num_users_to_spawn; i++) {
    if (pids[i] > 0) {
      waitpid(pids[i], NULL, 0);
    }
  }

  free(pids);
  detach_shm(worker_config);
  printf(COLOR_GREEN "Pulizia completata.\n" COLOR_RESET);

  return EXIT_SUCCESS;
}