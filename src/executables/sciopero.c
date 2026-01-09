/**
 * @file sciopero.c
 * @brief Tool per causare "Communication Disorder" (Sciopero).
 * Mostra lo stato attuale degli operatori e permette di bloccarli.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common/config.h"
#include "common/ipc_utils.h"
#include "common/logger.h"
#include "common/types.h"

/**
 * @brief Restituisce la rappresentazione testuale di un ruolo operatore.
 *
 * @param role Ruolo dell'operatore
 *
 * @return const char* Rappresentazione testuale del ruolo
 */
char *get_role_string(int role) {
  switch (role) {
  case OP_PRIMI:
    return "Primi";
  case OP_SECONDI:
    return "Secondi";
  case OP_CAFFE:
    return "Caffè";
  case OP_CASSA:
    return "Cassa";
  default:
    return "N/A";
  }
}

/**
 * @brief Rimuove i codici colore ANSI (es. \033[31m) da una stringa.
 * Modifica la stringa direttamente in memoria (in-place).
 *
 * * @param str La stringa da pulire (deve essere modificabile, char[], non
 * char*)
 */
void strip_ansi_codes(char *str) {
  char *src = str;
  char *dst = str;

  while (*src) {
    if (*src == '\033' && *(src + 1) == '[') {
      src += 2;

      while (*src &&
             !((*src >= 'A' && *src <= 'Z') || (*src >= 'a' && *src <= 'z'))) {
        src++;
      }

      if (*src) {
        src++;
      }
    } else {
      *dst++ = *src++;
    }
  }
  *dst = '\0';
}

/**
 * @brief Trims leading and trailing whitespace from a string
 */
static char *trim(char *str) {
  char *tmp;

  while (isspace((unsigned char)*str)) {
    str++;
  }

  if (*str == 0) {
    return str;
  }

  tmp = str + strlen(str) - 1;
  while (tmp > str && isspace((unsigned char)*tmp)) {
    tmp--;
  }
  *(tmp + 1) = 0;

  return str;
}

/**
 * @brief Converts a string to uppercase in place.
 * Supports ASCII and common Italian UTF-8 characters (à, è, é, ì, ò, ù).
 * * @param str The string to modify.
 */
void string_to_upper(char *str) {
  if (!str) {
    return;
  }

  while (*str) {
    unsigned char c = (unsigned char)*str;

    if (c >= 'a' && c <= 'z') {
      *str = (char)(c - 32);
    }

    else if (c == 0xC3) {
      unsigned char next = (unsigned char)*(str + 1);

      switch (next) {
      case 0xA0:
        *(str + 1) = (char)0x80;
        break; // à -> À
      case 0xA8:
        *(str + 1) = (char)0x88;
        break; // è -> È
      case 0xA9:
        *(str + 1) = (char)0x89;
        break; // é -> É
      case 0xAC:
        *(str + 1) = (char)0x8C;
        break; // ì -> Ì
      case 0xB2:
        *(str + 1) = (char)0x92;
        break; // ò -> Ò
      case 0xB9:
        *(str + 1) = (char)0x99;
        break; // ù -> Ù
      }

      str++;
    }

    str++;
  }
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  Config config;
  if (parse_config(argv[2], &config) == -1) {
    printf(COLOR_RED "Errore parsing config\n" COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  int shm_id = allocate_shm(sizeof(WorkerConfig), FTOK_SHM_ROLES_ID);
  if (shm_id == -1) {
    printf(COLOR_RED "Errore: Impossibile collegarsi alla SHM. Sistema non "
                     "avviato?\n" COLOR_RESET);
    return 1;
  }
  WorkerConfig *worker_config = (WorkerConfig *)attach_shm(shm_id);

  if (worker_config == NULL) {
    printf(COLOR_RED "Errore: Impossibile collegarsi alla SHM. Sistema non "
                     "avviato?\n" COLOR_RESET);
    return 1;
  }

  // clear screen
  printf("\033[H\033[J");

  printf("\n" COLOR_CYAN
         "=== GESTIONE SCIOPERI E STATO DEL PERSONALE ===" COLOR_RESET "\n");
  printf("Totale Operatori Configuarati: %d\n",
         worker_config->total_workers_count);
  printf("Giorno Corrente Simulazione: %d\n\n", worker_config->current_day);

  printf("╔════╦═════════════════════════════════════╦════════════╦════════════"
         "══════╗"
         "\n");
  printf("║ %-2s ║ %-36s ║ %-10s ║ %-16s ║\n", "ID", "IDENTITÀ (Nome & PID)",
         "MANSIONE", "STATO");
  printf("╠════╬═════════════════════════════════════╬════════════╬════════════"
         "══════╣"
         "\n");

  time_t now = time(NULL);
  int count_workers = worker_config->total_workers_count;

  for (int i = 0; i < count_workers; i++) {
    char *display_name = worker_config->worker_names[i];

    char safe_name[64];
    if (strlen(display_name) == 0) {
      snprintf(safe_name, sizeof(safe_name), "[inizializzazione...]");
    } else {
      snprintf(safe_name, sizeof(safe_name), "%s", display_name);
    }
    strip_ansi_codes(safe_name);

    int is_strike = (worker_config->strike_end_times[i] > now);

    char status_text[32];
    const char *color_code;
    const char *reset_code = COLOR_RESET;

    if (is_strike) {
      double remaining = difftime(worker_config->strike_end_times[i], now);
      snprintf(status_text, sizeof(status_text), "SCIOPERO (%.0fs)", remaining);
      color_code = COLOR_RED;
    } else {
      snprintf(status_text, sizeof(status_text), "ATTIVO");
      color_code = COLOR_GREEN;
    }

    const char *role = get_role_string(worker_config->worker_roles[i]);
    int padding_width = (strstr(role, "è") != NULL) ? 11 : 10;

    printf("║ %02d ║ %-35.35s ║ %-*.*s ║ %s%-16.16s%s ║\n", i, safe_name,
           padding_width, padding_width, role, color_code, status_text,
           reset_code);
  }
  printf("╚════╩═════════════════════════════════════╩════════════╩════════════"
         "══════╝"
         "\n");

  if (count_workers <= 0) {
    printf("\n" COLOR_YELLOW
           " * Simulazione in fase di inizializzazione..." COLOR_RESET "\n");
    printf("   (Nessun operatore rilevato in memoria condivisa)\n");
    detach_shm(worker_config);
    return 0;
  }

  printf("\n" COLOR_RED "=== MENU SCIOPERO ===" COLOR_RESET "\n");
  printf(
      " * Inserisci gli ID degli operatori da mandare in sciopero (separati da "
      "spazio).\n");
  printf("   Esempio: 0 2 3\n\n");
  printf("  > ");

  char input_buffer[256];
  if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
    detach_shm(worker_config);
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
    detach_shm(worker_config);
    return 0;
  }

  printf("\n\n * Inserisci la durata dello sciopero in secondi (%d):\n\n  > ",
         config.default_sciopero_stop_duration);

  int duration = config.default_sciopero_stop_duration;
  char duration_buf[64];

  if (fgets(duration_buf, sizeof(duration_buf), stdin) != NULL) {
    if (duration_buf[0] != '\n') {
      int parsed_val = atoi(duration_buf);
      if (parsed_val > 0) {
        duration = parsed_val;
      }
    }
  }

  time_t end_time = time(NULL) + duration;

  printf("\n" COLOR_RED "! SCIOPERO INDETTO (%d sec) !" COLOR_RESET "\n\n",
         duration);

  for (int i = 0; i < target_count; i++) {
    int id = targets[i];
    if (id >= 0 && id < MAX_WORKERS) {
      worker_config->strike_end_times[id] = end_time;

      char safe_name[64];
      snprintf(safe_name, sizeof(safe_name), "%s",
               worker_config->worker_names[id]);
      strip_ansi_codes(safe_name);

      char role_str[16];
      const char *temp_role = get_role_string(worker_config->worker_roles[id]);
      strncpy(role_str, temp_role, sizeof(role_str) - 1);
      role_str[sizeof(role_str) - 1] = '\0';

      string_to_upper(role_str);

      printf(" ➯ Operatore %d (" COLOR_GREEN "%s" COLOR_RESET
             " | Postazione %s)\n",
             id, trim(safe_name), role_str);

      printf(COLOR_RED "   BLOCCATO fino a `%s`" COLOR_RESET "\n\n",
             trim(ctime(&end_time)));
    } else {
      printf(" ➯ ID %d non valido, ignorato.\n", id);
    }
  }

  detach_shm(worker_config);
  return 0;
}