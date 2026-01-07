/**
 * @file sciopero.c
 * @brief Tool per causare "Communication Disorder" (Sciopero).
 * Mostra lo stato attuale degli operatori e permette di bloccarli.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common/config.h"
#include "common/ipc_utils.h"
#include "common/logger.h"
#include "common/types.h"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  int shm_id = allocate_shm(sizeof(WorkerConfig), FTOK_SHM_ROLES_ID);
  if (shm_id == -1) {
    printf(COLOR_RED "Errore: Impossibile collegarsi alla SHM. Sistema non "
                     "avviato?\n" COLOR_RESET);
    return 1;
  }
  WorkerConfig *cfg = (WorkerConfig *)attach_shm(shm_id);

  // clear screen
  printf("\033[H\033[J");

  printf("\n" COLOR_CYAN "=== STATO OPERATORI ===" COLOR_RESET "\n");
  printf("Totale Operatori Configuarati: %d\n", cfg->total_workers_count);
  printf("--------------------------------------------------\n");
  printf("ID\t| RUOLO ATTUALE\t| STATO\n");
  printf("--------------------------------------------------\n");

  time_t now = time(NULL);
  int count_workers = cfg->total_workers_count;

  if (count_workers <= 0) {
    count_workers = MAX_WORKERS;
  }

  for (int i = 0; i < count_workers; i++) {
    int is_strike = (cfg->strike_end_times[i] > now);
    const char *status = is_strike ? COLOR_RED "SCIOPERO" COLOR_RESET
                                   : COLOR_GREEN "ATTIVO" COLOR_RESET;

    printf("%d\t| %-10s\t| %s", i, ROLE_NAME(cfg->worker_roles[i]), status);

    if (is_strike) {
      double remaining = difftime(cfg->strike_end_times[i], now);
      printf(" (Ancora %.0fs)", remaining);
    }
    printf("\n");
  }
  printf("--------------------------------------------------\n");

  printf("\n" COLOR_RED "=== MENU SCIOPERO ===" COLOR_RESET "\n");
  printf(" * Inserisci gli ID degli operatori da mandare in sciopero (separati da "
         "spazio).\n");
  printf("   Esempio: 0 2 3\n");
  printf("> ");

  char input_buffer[256];
  if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
    detach_shm(cfg);
    return 0;
  }

  int targets[MAX_WORKERS];
  int target_count = 0;

  char *token = strtok(input_buffer, " \n");
  while (token != NULL && target_count < MAX_WORKERS) {
    targets[target_count++] = atoi(token);
    token = strtok(NULL, " \n");
  }

  if (target_count == 0) {
    printf("Nessun target selezionato. Uscita.\n");
    detach_shm(cfg);
    return 0;
  }

  printf(" * Inserisci la durata dello sciopero in secondi:\n> ");
  int duration = 0;
  if (scanf("%d", &duration) != 1 || duration <= 0)
    duration = 5;

  time_t end_time = time(NULL) + duration;

  printf("\n" COLOR_RED "!!! APPLICAZIONE SCIOPERO (%d sec) !!!" COLOR_RESET
         "\n",
         duration);

  for (int i = 0; i < target_count; i++) {
    int id = targets[i];
    if (id >= 0 && id < MAX_WORKERS) {
      // Aggiornamento atomico del timestamp
      cfg->strike_end_times[id] = end_time;
      printf(" -> Operatore %d (%s): BLOCCATO fino a %s", id,
             ROLE_NAME(cfg->worker_roles[id]), ctime(&end_time));
    } else {
      printf(" -> ID %d non valido, ignorato.\n", id);
    }
  }

  detach_shm(cfg);
  printf("\nComando inviato. Gli operatori si fermeranno alla prossima "
         "richiesta.\n");
  return 0;
}