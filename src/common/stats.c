#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/logger.h"
#include "common/stats.h"

/**
 * @brief Unisce cartella e nome file gestendo correttamente gli slash.
 * @return Stringa allocata dinamicamente (da liberare con free).
 */
static char *join_path(const char *folder, const char *filename) {
  if (!folder || strlen(folder) == 0) {
    return strdup(filename);
  }

  size_t len_folder = strlen(folder);

  int folder_ends_slash = (folder[len_folder - 1] == '/');
  int file_starts_slash = (filename[0] == '/');

  char *result;

  if (folder_ends_slash && file_starts_slash) {
    // "folder/" + "/file" => "folder/file" (salta il primo char di file)
    asprintf(&result, "%s%s", folder, filename + 1);
  } else if (!folder_ends_slash && !file_starts_slash) {
    // "folder" + "file" => "folder/file" (aggiungi slash)
    asprintf(&result, "%s/%s", folder, filename);
  } else {
    // "folder/" + "file" oppure "folder" + "/file" => concatena
    asprintf(&result, "%s%s", folder, filename);
  }

  return result;
}

/**
 * @brief Crea ricorsivamente le directory (equivalente a mkdir -p)
 */
static int mkdir_p(const char *path) {
  char *temp_path = strdup(path);
  char *p = temp_path;

  // salta il primo slash se path è assoluto
  if (*p == '/') {
    p++;
  }

  for (; *p; p++) {
    if (*p == '/') {
      *p = 0;

      if (mkdir(temp_path, 0755) != 0) {
        if (errno != EEXIST) {
          free(temp_path);
          return -1;
        }
      }
      *p = '/';
    }
  }

  if (mkdir(temp_path, 0755) != 0) {
    if (errno != EEXIST) {
      free(temp_path);
      return -1;
    }
  }

  free(temp_path);
  return 0;
}

/**
 * @brief Assicura che la cartella padre del file esista.
 */
static int ensure_folder_exists(const char *full_file_path) {
  char *path_copy = strdup(full_file_path);
  char *dir = dirname(path_copy);

  if (strcmp(dir, ".") != 0 && strcmp(dir, "/") != 0) {
    struct stat st = {0};
    if (stat(dir, &st) == -1) {
      if (mkdir_p(dir) == -1) {
        LOG_ERR("STATS", "Impossibile creare directory: %s", dir);
        free(path_copy);
        return -1;
      } else {
        LOG_INFO("STATS", "Creata directory export: %s", dir);
      }
    }
  }
  free(path_copy);
  return 0;
}

/**
 * @brief Assicura che l'header esista.
 * Se il file non esiste, lo crea e scrive l'header.
 */
static void ensure_csv_header(const char *full_path) {
  if (access(full_path, F_OK) != -1) {
    return;
  }

  FILE *f = fopen(full_path, "w");
  if (!f) {
    return;
  }

  fprintf(f, "Tipo,Giorno,Utenti_Serviti,Utenti_Respinti,Primi,Secondi,Caffe,"
             "Ricavo,Attesa_Primi,Attesa_Secondi,Attesa_Caffe,Attesa_Cassa,"
             "Transazioni,Leftover_Primi,Leftover_Secondi,Leftover_Caffe\n");
  fclose(f);
}

/**
 * @brief Appende una riga di statistiche al CSV.
 *
 * @param f File pointer aperto in modalità append.
 * @param type Stringa identificativa ("GIORNALIERO" o "GLOBALE").
 * @param day Numero del giorno (o -1 per globale).
 * @param served Utenti serviti.
 * @param refused Utenti respinti.
 * @param p Piatti Primi.
 * @param s Piatti Secondi.
 * @param c Piatti Caffè.
 * @param rev Ricavo.
 * @param w_p Tempo attesa primi.
 * @param w_s Tempo attesa secondi.
 * @param w_c Tempo attesa caffè.
 * @param w_ca Tempo attesa cassa.
 * @param trans Numero transazioni.
 * @param l_p Avanzi primi (opzionale, -1 se non applicabile).
 * @param l_s Avanzi secondi (opzionale, -1 se non applicabile).
 * @param l_c Avanzi caffè (opzionale, -1 se non applicabile).
 */
static void write_stats_row(FILE *f, const char *type, int day, int served,
                            int refused, int p, int s, int c, double rev,
                            double w_p, double w_s, double w_c, double w_ca,
                            int trans, int l_p, int l_s, int l_c) {

  fprintf(f, "%s,%d,%d,%d,%d,%d,%d,%.2f,%.4f,%.4f,%.4f,%.4f,%d,%d,%d,%d\n",
          type, day, served, refused, p, s, c, rev, w_p, w_s, w_c, w_ca, trans,
          l_p, l_s, l_c);
}

/**
 * @brief Helper per generare il path completo con suffisso (es. -day_1.csv)
 */
static char *generate_csv_path(const char *folder, const char *prefix,
                               int day_num, int is_final) {
  char *filename;
  if (is_final) {
    asprintf(&filename, "%s.csv", prefix);
  } else {
    asprintf(&filename, "%s-day_%d.csv", prefix, day_num);
  }

  char *full_path = join_path(folder, filename);
  free(filename);
  return full_path;
}

/**
 * @brief Helper interno per calcolare e scrivere una riga di statistiche.
 * Usato sia per daily che per final.
 */
static void append_stats_to_file(const char *full_path, const char *row_type,
                                 int day_num, int served, int refused, int p,
                                 int s, int c, double rev, double w_p,
                                 double w_s, double w_c, double w_ca, int trans,
                                 int l_p, int l_s, int l_c) {

  if (ensure_folder_exists(full_path) == -1) {
    return;
  }
  ensure_csv_header(full_path);

  FILE *f = fopen(full_path, "a");
  if (!f) {
    LOG_ERR("STATS", "Errore append su '%s': %s", full_path, strerror(errno));
    return;
  }
  write_stats_row(f, row_type, day_num, served, refused, p, s, c, rev, w_p, w_s,
                  w_c, w_ca, trans, l_p, l_s, l_c);
  fclose(f);
}

char *process_daily_report(DailyReport *report, GlobalStats *total_stats) {
  char *buffer = NULL;

  double avg_p = (report->daily_plates_primi > 0)
                     ? report->daily_wait_primi / report->daily_plates_primi
                     : 0.0;
  double avg_s = (report->daily_plates_secondi > 0)
                     ? report->daily_wait_secondi / report->daily_plates_secondi
                     : 0.0;
  double avg_c = (report->daily_plates_caffe > 0)
                     ? report->daily_wait_caffe / report->daily_plates_caffe
                     : 0.0;
  double avg_ca = (report->daily_transactions > 0)
                      ? report->daily_wait_cassa / report->daily_transactions
                      : 0.0;

  int len = asprintf(&buffer,
                     "\n" COLOR_BLUE
                     "========== REPORT GIORNO %d ==========" COLOR_RESET "\n"
                     "Utenti Serviti:   %d\n"
                     "Utenti Respinti:  %d\n"
                     "Piatti Distribuiti:\n"
                     "  - Primi:   %d (Avanzi: %d)\n"
                     "  - Secondi: %d (Avanzi: %d)\n"
                     "  - Caffè:   %d\n"
                     "Ricavo Giornata:  %.2f€\n" COLOR_CYAN
                     "======== Totali Accumulati =========\n" COLOR_RESET
                     "Totale Serviti:   %d\n"
                     "Totale Ricavi:    %.2f€\n" COLOR_CYAN
                     "====== Tempi Medi Attesa (s) =======\n" COLOR_RESET
                     "  - Primi:   %.4f s\n"
                     "  - Secondi: %.4f s\n"
                     "  - Caffè:   %.4f s\n"
                     "  - Cassa:   %.4f s\n" COLOR_BLUE
                     "====================================" COLOR_RESET "\n",
                     report->day_number, report->daily_users_served,
                     report->daily_users_refused, report->daily_plates_primi,
                     report->leftover_primi, report->daily_plates_secondi,
                     report->leftover_secondi, report->daily_plates_caffe,
                     report->daily_revenue, total_stats->total_users_served,
                     total_stats->total_revenue, avg_p, avg_s, avg_c, avg_ca);

  if (len == -1) {
    return NULL;
  }

  return buffer;
}

char *process_final_report(GlobalStats *stats, int total_days) {
  char *buffer = NULL;

  double avg_p = (stats->total_plates_primi > 0)
                     ? stats->total_wait_time_primi / stats->total_plates_primi
                     : 0.0;
  double avg_s =
      (stats->total_plates_secondi > 0)
          ? stats->total_wait_time_secondi / stats->total_plates_secondi
          : 0.0;
  double avg_c = (stats->total_plates_caffe > 0)
                     ? stats->total_wait_time_caffe / stats->total_plates_caffe
                     : 0.0;
  double avg_ca = (stats->total_transactions > 0)
                      ? stats->total_wait_time_cassa / stats->total_transactions
                      : 0.0;

  int len = asprintf(
      &buffer,
      "\n" COLOR_BLUE "======== REPORT FINALE =========" COLOR_RESET "\n"
      "Giorni Completati: %d\n"
      "Piatti Serviti in totale: %d\n"
      "Utenti Respinti/Overload: %d\n"
      "Piatti Distribuiti:\n"
      "  - Primi: %d\n"
      "  - Secondi: %d\n"
      "  - Caffè: %d\n" COLOR_CYAN
      "======= Totali Accumulati =======\n" COLOR_RESET
      "Transazioni Totali: %d\n"
      "Ricavo Totale: %.2f€\n" COLOR_CYAN
      "===== Tempi Medi Attesa (s) =====\n" COLOR_RESET "  - Primi:   %.4f s\n"
      "  - Secondi: %.4f s\n"
      "  - Caffè:   %.4f s\n"
      "  - Cassa:   %.4f s\n" COLOR_BLUE
      "================================" COLOR_RESET "\n",
      total_days, stats->total_users_served, stats->total_users_refused,
      stats->total_plates_primi, stats->total_plates_secondi,
      stats->total_plates_caffe, stats->total_transactions,
      stats->total_revenue, avg_p, avg_s, avg_c, avg_ca);

  if (len == -1) {
    return NULL;
  }

  return buffer;
}

void export_daily_stats_to_csv(DailyReport *report, const char *folder_path,
                               const char *file_prefix,
                               const char *final_file_prefix) {

  // calcolo medie (codice comune)
  double avg_p = (report->daily_plates_primi > 0)
                     ? report->daily_wait_primi / report->daily_plates_primi
                     : 0.0;
  double avg_s = (report->daily_plates_secondi > 0)
                     ? report->daily_wait_secondi / report->daily_plates_secondi
                     : 0.0;
  double avg_c = (report->daily_plates_caffe > 0)
                     ? report->daily_wait_caffe / report->daily_plates_caffe
                     : 0.0;
  double avg_ca = (report->daily_transactions > 0)
                      ? report->daily_wait_cassa / report->daily_transactions
                      : 0.0;

  char *day_path =
      generate_csv_path(folder_path, file_prefix, report->day_number, 0);
  unlink(day_path);

  append_stats_to_file(day_path, "GIORNALIERO", report->day_number,
                       report->daily_users_served, report->daily_users_refused,
                       report->daily_plates_primi, report->daily_plates_secondi,
                       report->daily_plates_caffe, report->daily_revenue, avg_p,
                       avg_s, avg_c, avg_ca, report->daily_transactions,
                       report->leftover_primi, report->leftover_secondi,
                       report->leftover_caffe);
  free(day_path);

  // scrivo ANCHE sul file cumulativo finale (es. final_stats.csv)
  if (final_file_prefix && strlen(final_file_prefix) > 0) {
    char *final_path = generate_csv_path(folder_path, final_file_prefix, 0, 1);

    append_stats_to_file(
        final_path, "GIORNALIERO", report->day_number,
        report->daily_users_served, report->daily_users_refused,
        report->daily_plates_primi, report->daily_plates_secondi,
        report->daily_plates_caffe, report->daily_revenue, avg_p, avg_s, avg_c,
        avg_ca, report->daily_transactions, report->leftover_primi,
        report->leftover_secondi, report->leftover_caffe);
    free(final_path);
  }
}

void export_final_stats_to_csv(GlobalStats *total_stats, int total_days,
                               const char *folder_path,
                               const char *file_prefix) {

  char *full_path = generate_csv_path(folder_path, file_prefix, 0, 1);

  double avg_p =
      (total_stats->total_plates_primi > 0)
          ? total_stats->total_wait_time_primi / total_stats->total_plates_primi
          : 0.0;
  double avg_s = (total_stats->total_plates_secondi > 0)
                     ? total_stats->total_wait_time_secondi /
                           total_stats->total_plates_secondi
                     : 0.0;
  double avg_c =
      (total_stats->total_plates_caffe > 0)
          ? total_stats->total_wait_time_caffe / total_stats->total_plates_caffe
          : 0.0;
  double avg_ca =
      (total_stats->total_transactions > 0)
          ? total_stats->total_wait_time_cassa / total_stats->total_transactions
          : 0.0;

  append_stats_to_file(
      full_path, "GLOBALE_FINALE", total_days, total_stats->total_users_served,
      total_stats->total_users_refused, total_stats->total_plates_primi,
      total_stats->total_plates_secondi, total_stats->total_plates_caffe,
      total_stats->total_revenue, avg_p, avg_s, avg_c, avg_ca,
      total_stats->total_transactions, -1, -1, -1);

  LOG_INFO("STATS", "Statistiche finali aggiornate su %s", full_path);
  free(full_path);
}