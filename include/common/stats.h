#ifndef STATS_H
#define STATS_H

#include "types.h"
#include <stdbool.h>

typedef struct global_stats {
  int total_users_served;
  int total_users_refused;
  int total_users_w_ticket;

  // piatti distribuiti
  int total_plates_primi;
  int total_plates_secondi;
  int total_plates_caffe;

  // accumulatori tempo di attesa
  double total_wait_time_primi;
  double total_wait_time_secondi;
  double total_wait_time_caffe;
  double total_wait_time_cassa;

  // leftover
  int total_leftover_primi;
  int total_leftover_secondi;
  int total_leftover_caffe;

  // refill
  int total_refilled_primi;
  int total_refilled_secondi;
  int total_refilled_caffe;

  double total_revenue;
  int total_transactions;

  int total_user_poverty;
} GlobalStats;

typedef struct daily_report {
  int day_number;

  // delta (oggi - ieri)
  int daily_users_served;
  int daily_users_refused;
  int daily_users_w_ticket;

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

  int daily_refilled_primi;
  int daily_refilled_secondi;
  int daily_refilled_caffe;

  // avanzi
  int leftover_primi;
  int leftover_secondi;
  int leftover_caffe;

  int daily_user_poverty;
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
 * @brief Crea il file del giorno E appende la riga al report cumulativo.
 *
 * @param report Dati del giorno
 * @param folder_path Cartella base
 * @param day_file_prefix Prefisso file giorno (es. "days/daily_report")
 * @param final_file_prefix Prefisso file finale per append (es. "final_stats")
 */
void export_daily_stats_to_csv(DailyReport *report, const char *folder_path,
                               const char *day_file_prefix,
                               const char *final_file_prefix,
                               const bool create_daily_single_files);

/**
 * @brief Appende la riga globale al file finale.
 *
 * @param total_stats Dati globali
 * @param folder_path Cartella base
 * @param final_file_prefix Prefisso file finale
 */
void export_final_stats_to_csv(GlobalStats *total_stats, const int total_days,
                               const char *folder_path,
                               const char *final_file_prefix);
#endif