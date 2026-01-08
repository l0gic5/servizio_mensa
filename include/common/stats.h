#ifndef STATS_H
#define STATS_H

#include "types.h"

typedef struct global_stats {
  int total_users_served;
  int total_users_refused;

  // piatti distribuiti
  int total_plates_primi;
  int total_plates_secondi;
  int total_plates_caffe;

  // accumulatori tempo di attesa
  double total_wait_time_primi;
  double total_wait_time_secondi;
  double total_wait_time_caffe;
  double total_wait_time_cassa;

  double total_revenue;
  int total_transactions;
} GlobalStats;

typedef struct daily_report {
  int day_number;

  // delta (oggi - ieri)
  int daily_users_served;
  int daily_users_refused;

  int daily_plates_primi;
  int daily_plates_secondi;
  int daily_plates_caffe;

  // delta tempi attesa
  double daily_wait_primi;
  double daily_wait_secondi;
  double daily_wait_caffe;
  double daily_wait_cassa;

  double daily_revenue;
  int daily_transactions;

  // avanzi
  int leftover_primi;
  int leftover_secondi;
  int leftover_caffe;
} DailyReport;

/**
 * @brief Processa il report giornaliero e ritorna una stringa formattata
 * pronta per la stampa su console.
 *
 * @warning NON gestisce MUTEX `sem_wait`/`sem_signal`
 *
 * @param report Report giornaliero da processare
 * @param total_stats Statistiche globali aggiornate
 *
 * @return Stringa formattata con il report giornaliero
 */
char *process_daily_report(DailyReport *report, GlobalStats *total_stats);

/**
 * @brief Processa il report finale e ritorna una stringa formattata
 * pronta per la stampa su console.
 *
 * @warning NON gestisce MUTEX `sem_wait`/`sem_signal`
 *
 * @param report Report finale da processare
 *
 * @return Stringa formattata con il report finale
 */
char *process_final_report(GlobalStats *stats, int total_days);

/**
 * @brief Crea un file CSV per il giorno corrente (es. report-day_1.csv).
 *
 * @param report Report giornaliero da esportare
 * @param folder_path Cartella di destinazione
 * @param filename_prefix Prefisso del nome file (es. "report")
 */
void export_daily_stats_to_csv(DailyReport *report, const char *folder_path,
                               const char *filename_prefix,
                               const char *final_file_prefix_for_append);

/**
 * @brief Esporta le statistiche finali in un file CSV.
 *
 * @param total_stats Statistiche globali finali
 * @param folder_path Cartella di destinazione
 * @param filename_prefix Prefisso del nome file (es. "final_stats")
 */
void export_final_stats_to_csv(GlobalStats *total_stats, int total_days,
                               const char *folder_path,
                               const char *filename_prefix);
#endif