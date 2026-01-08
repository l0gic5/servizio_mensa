#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/logger.h"
#include "common/stats.h"

static const char *CSV_HEADER =
    "Tipo,Giorno,"
    "Utenti_Serviti,Utenti_Respinti,"
    "Primi,Secondi,Caffe,"
    "Ricavo,"
    "Attesa_Primi,Attesa_Secondi,Attesa_Caffe,Attesa_Cassa,"
    "Transazioni,"
    "Leftover_Primi,Leftover_Secondi,Leftover_Caffe"
    "\n";

static const char *CSV_FORMAT = "%s,%d,"               // Tipo, Giorno
                                "%d,%d,"               // Serviti, Respinti
                                "%d,%d,%d,"            // Piatti
                                "%.2f,"                // Ricavo
                                "%.4f,%.4f,%.4f,%.4f," // Tempi Attesa
                                "%d,"                  // Transazioni
                                "%d,%d,%d"             // Leftovers
                                "\n";

/**
 * @brief Calcola la media sicura.
 * Gestisce divisione per zero e ignora valori negativi non validi.
 *
 * @param total_value Valore totale accumulato.
 * @param count Numero di elementi.
 *
 * @return La media calcolata, o 0.0 in caso di errori.
 */
static double calculate_avg(double total, int count) {
  return (count > 0 && total > 0.0) ? (total / count) : 0.0;
}

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
 * @brief Assicura che l'header esista nel file (utile per append).
 */
static void ensure_csv_header(const char *path) {
  if (access(path, F_OK) == 0) {
    return;
  }

  FILE *f = fopen(path, "w");
  if (f) {
    fprintf(f, "%s", CSV_HEADER);
    fclose(f);
  }
}

/**
 * @brief Scrive una riga nel file CSV.
 *
 * @param f File pointer
 * @param type Tipo di report (DAILY o FINAL)
 * @param day Giorno del report (0 per finale)
 * @param served Utenti serviti
 * @param refused Utenti respinti
 * @param p Primi serviti
 * @param s Secondi serviti
 * @param c Caffè serviti
 * @param rev Ricavo
 * @param w_p Tempo attesa primi
 * @param w_s Tempo attesa secondi
 * @param w_c Tempo attesa caffè
 * @param w_ca Tempo attesa cassa
 * @param trans Transazioni
 * @param l_p Leftover primi
 * @param l_s Leftover secondi
 * @param l_c Leftover caffè
 */
static void write_csv_row(FILE *f, const char *type, int day, int served,
                          int refused, int p, int s, int c, double rev,
                          double w_p, double w_s, double w_c, double w_ca,
                          int trans, int l_p, int l_s, int l_c) {
  if (fprintf(f, CSV_FORMAT, type, day, served, refused, p, s, c, rev, w_p, w_s,
              w_c, w_ca, trans, l_p, l_s, l_c) < 0) {
    LOG_ERR("STATS", "Errore scrittura riga CSV: %s", strerror(errno));
  }
}

/**
 * @brief Funzione generica per appendere una riga di statistiche a un file.
 * Gestisce apertura, header (se nuovo) e chiusura.
 *
 * @param full_path Percorso completo del file CSV
 * @param row_type Tipo di riga (DAILY o FINAL)
 * @param day Giorno del report (0 per finale)
 * @param served Utenti serviti
 * @param refused Utenti respinti
 * @param p Primi serviti
 * @param s Secondi serviti
 * @param c Caffè serviti
 * @param rev Ricavo
 * @param w_p Tempo attesa primi
 * @param w_s Tempo attesa secondi
 * @param w_c Tempo attesa caffè
 * @param w_ca Tempo attesa cassa
 * @param trans Transazioni
 * @param l_p Leftover primi
 * @param l_s Leftover secondi
 * @param l_c Leftover caffè
 */
static void append_stats_to_file(const char *full_path, const char *row_type,
                                 int day, int served, int refused, int p, int s,
                                 int c, double rev, double w_p, double w_s,
                                 double w_c, double w_ca, int trans, int l_p,
                                 int l_s, int l_c) {

  if (ensure_folder_exists(full_path) == -1) {
    return;
  }

  ensure_csv_header(full_path);

  // append
  FILE *f = fopen(full_path, "a");
  if (!f) {
    LOG_ERR("STATS", "Errore append su '%s': %s", full_path, strerror(errno));
    return;
  }

  write_csv_row(f, row_type, day, served, refused, p, s, c, rev, w_p, w_s, w_c,
                w_ca, trans, l_p, l_s, l_c);
  fclose(f);
}

char *process_daily_report(DailyReport *report, GlobalStats *total_stats) {
  char *buffer = NULL;

  double avg_p =
      calculate_avg(report->daily_wait_primi, report->daily_plates_primi);
  double avg_s =
      calculate_avg(report->daily_wait_secondi, report->daily_plates_secondi);
  double avg_c =
      calculate_avg(report->daily_wait_caffe, report->daily_plates_caffe);
  double avg_ca =
      calculate_avg(report->daily_wait_cassa, report->daily_transactions);

  int len = asprintf(&buffer,
                     "\n" COLOR_BLUE
                     "========== REPORT GIORNO %d ==========" COLOR_RESET "\n"
                     "Utenti Serviti:   %d\n"
                     "  * con ticket: (%d/%d)\n"
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
                     report->daily_users_w_ticket, report->daily_users_served,
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

  double avg_p =
      calculate_avg(stats->total_wait_time_primi, stats->total_plates_primi);
  double avg_s = calculate_avg(stats->total_wait_time_secondi,
                               stats->total_plates_secondi);
  double avg_c =
      calculate_avg(stats->total_wait_time_caffe, stats->total_plates_caffe);
  double avg_ca =
      calculate_avg(stats->total_wait_time_cassa, stats->total_transactions);

  int len = asprintf(
      &buffer,
      "\n" COLOR_BLUE "======== REPORT FINALE =========" COLOR_RESET "\n"
      "Giorni Completati: %d\n"
      "Utenti Serviti in totale: %d\n"
      "  * con ticket: (%d/%d)\n"
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
      total_days, stats->total_users_served, stats->total_users_w_ticket,
      stats->total_users_served, stats->total_users_refused,
      stats->total_plates_primi, stats->total_plates_secondi,
      stats->total_plates_caffe, stats->total_transactions,
      stats->total_revenue, avg_p, avg_s, avg_c, avg_ca);

  if (len == -1) {
    return NULL;
  }

  return buffer;
}

void export_daily_stats_to_csv(DailyReport *report, const char *folder_path,
                               const char *day_file_prefix,
                               const char *final_file_prefix) {

  double avg_p =
      calculate_avg(report->daily_wait_primi, report->daily_plates_primi);
  double avg_s =
      calculate_avg(report->daily_wait_secondi, report->daily_plates_secondi);
  double avg_c =
      calculate_avg(report->daily_wait_caffe, report->daily_plates_caffe);
  double avg_ca =
      calculate_avg(report->daily_wait_cassa, report->daily_transactions);

  char *day_path =
      generate_csv_path(folder_path, day_file_prefix, report->day_number, 0);
  if (ensure_folder_exists(day_path) != -1) {
    FILE *f = fopen(day_path, "w");
    if (f) {
      fprintf(f, "%s", CSV_HEADER);
      write_csv_row(f, "GIORNALIERO", report->day_number,
                    report->daily_users_served, report->daily_users_refused,
                    report->daily_plates_primi, report->daily_plates_secondi,
                    report->daily_plates_caffe, report->daily_revenue, avg_p,
                    avg_s, avg_c, avg_ca, report->daily_transactions,
                    report->leftover_primi, report->leftover_secondi,
                    report->leftover_caffe);
      fclose(f);
    } else {
      LOG_ERR("STATS", "Errore creazione CSV daily '%s': %s", day_path,
              strerror(errno));
    }
  }
  free(day_path);

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

void export_final_stats_to_csv(GlobalStats *total_stats, const int total_days,
                               const char *folder_path,
                               const char *final_file_prefix) {

  char *final_path = generate_csv_path(folder_path, final_file_prefix, 0, 1);

  double avg_p = calculate_avg(total_stats->total_wait_time_primi,
                               total_stats->total_plates_primi);
  double avg_s = calculate_avg(total_stats->total_wait_time_secondi,
                               total_stats->total_plates_secondi);
  double avg_c = calculate_avg(total_stats->total_wait_time_caffe,
                               total_stats->total_plates_caffe);
  double avg_ca = calculate_avg(total_stats->total_wait_time_cassa,
                                total_stats->total_transactions);

  append_stats_to_file(
      final_path, "GLOBALE_FINALE", total_days, total_stats->total_users_served,
      total_stats->total_users_refused, total_stats->total_plates_primi,
      total_stats->total_plates_secondi, total_stats->total_plates_caffe,
      total_stats->total_revenue, avg_p, avg_s, avg_c, avg_ca,
      total_stats->total_transactions, -1, -1, -1);

  LOG_INFO("STATS", "Statistiche finali (Globale) aggiunte su %s", final_path);
  free(final_path);
}