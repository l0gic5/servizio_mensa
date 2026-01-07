#ifndef CONFIG_H
#define CONFIG_H

#include "common/logger.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 512

typedef enum cassa_position { INGRESSO, USCITA } CassaPosition;

typedef struct config {
  ///////////////////////////////////////////////
  //  1) SIMULAZIONE GLOBALE (Tempo e Limiti)  //
  ///////////////////////////////////////////////

  int simulation_duration_days; // SIM_DURATION
  int n_nanosecs_as_minute;     // N_NANO_SECS (Time scaling)
  int daily_service_minutes;    // DAILY_SERVICE_MINUTES
  int system_startup_delay_sec; // SYSTEM_STARTUP_DELAY_SEC
  int overload_threshold;       // OVERLOAD_THRESHOLD
  CassaPosition cassa_position; // CASSA_POSITION

  ////////////////////////////////////////
  //  2) POPOLAZIONE E RISORSE FISICHE  //
  ////////////////////////////////////////

  int nof_users;       // NOF_USERS
  int nof_workers;     // NOF_WORKERS
  int nof_table_seats; // NOF_TABLE_SEATS (Posti a sedere totali)

  ///////////////////////////////////////////////////
  //  3) CONFIGURAZIONE STAZIONI (Code e Banconi)  //
  ///////////////////////////////////////////////////

  // capacità code (utenti in attesa)
  int queue_capacity_primi;   // NOF_WK_SEATS_PRIMI
  int queue_capacity_secondi; // NOF_WK_SEATS_SECONDI
  int queue_capacity_caffe;   // NOF_WK_SEATS_CAFFE
  int queue_capacity_cassa;   // NOF_WK_SEATS_CASSA

  // postazioni fisiche (numero operatori contemporanei)
  int workstations_primi;   // WORKSTATIONS_PRIMI
  int workstations_secondi; // WORKSTATIONS_SECONDI
  int workstations_caffe;   // WORKSTATIONS_CAFFE
  int workstations_cassa;   // WORKSTATIONS_CASSA

  ////////////////////////////////////////////////////////////
  //  4) METRICHE DI SERVIZIO (Tempi, Variabilità, Prezzi)  //
  ////////////////////////////////////////////////////////////

  // tempi medi base
  int avg_service_primi;   // AVG_SRVC_PRIMI
  int avg_service_secondi; // AVG_SRVC_SECONDI
  int avg_service_caffe;   // AVG_SRVC_CAFFE
  int avg_service_cassa;   // AVG_SRVC_CASSA

  // variabilità (% +/-)
  int variability_primi;   // VARIABILITY_PRIMI
  int variability_secondi; // VARIABILITY_SECONDI
  int variability_caffe;   // VARIABILITY_CAFFE
  int variability_cassa;   // VARIABILITY_CASSA

  // prezzi
  double price_primi;   // PRICE_PRIMI
  double price_secondi; // PRICE_SECONDI
  double price_caffe;   // PRICE_CAFFE

  //////////////////////////////////////////
  //  5) COMPORTAMENTO OPERATORI (Pause)  //
  //////////////////////////////////////////

  int max_pauses_per_day;        // MAX_PAUSES_PER_DAY
  int pause_duration_ns;         // PAUSE_DURATION_NS
  int pause_probability_percent; // PAUSE_PROBABILITY_PERCENT

  ////////////////////////////////////////////
  //  6) COMPORTAMENTO E PREFERENZE UTENTI  //
  ////////////////////////////////////////////

  // logica
  int user_queue_timeout_sec;    // USER_QUEUE_TIMEOUT_SEC
  int user_meal_duration_ns;     // USER_MEAL_DURATION_NS
  int user_max_arrival_delay_us; // USER_MAX_ARRIVAL_DELAY_US

  // preferenze (%)
  int probability_user_wants_primo;   // PROBABILITY_USER_WANTS_PRIMO
  int probability_user_wants_secondo; // PROBABILITY_USER_WANTS_SECONDO
  int probability_user_wants_caffe;   // PROBABILITY_USER_WANTS_CAFFE

  // budget
  int user_budget_min;       // USER_BUDGET_MIN
  int user_budget_max;       // USER_BUDGET_MAX
  int user_min_daily_salary; // USER_MIN_DAILY_SALARY
  int user_max_daily_salary; // USER_MAX_DAILY_SALARY

  ///////////////////////////////////
  //  7) LOGISTICA (Rifornimenti)  //
  ///////////////////////////////////

  int refill_interval_minutes; // REFILL_INTERVAL_MINUTES
  int avg_refill_primi;     // AVG_REFILL_PRIMI
  int avg_refill_secondi;   // AVG_REFILL_SECONDI
  int max_porzioni_primi;   // MAX_PORZIONI_PRIMI
  int max_porzioni_secondi; // MAX_PORZIONI_SECONDI
  int max_porzioni_caffe;   // MAX_PORZIONI_CAFFE

} Config;

/**
 * @brief Parsing del file di configurazione.
 *
 * Inizializza la struttura Config a zero e legge i parametri
 * dal file indicato.
 *
 * @param filename Percorso del file di configurazione
 * @param config Puntatore alla struttura Config da riempire
 * @return 0 se il parsing va a buon fine, -1 in caso di errore
 */
int parse_config(const char *filename, Config *config);

/**
 * @brief Genera un numero casuale (logica intera castata a double)
 * * @param min Valore minimo
 * @param max Valore massimo
 * @param generator Puntatore alla funzione di generazione (es. rand)
 * @return double Risultato
 */
double random_range(double min, double max, int (*generator)(void));

/**
 * @brief Restituisce true con una certa probabilità percentuale.
 *
 * @param percent Probabilità in percentuale (0-100)
 * @param generator Puntatore alla funzione di generazione (es. rand)
 *
 * @return true Se l'evento si verifica
 * @return false Altrimenti
 */
bool random_probability(int percent, int (*generator)(void));

/**
 * @brief Calcola un valore casuale applicando una varianza percentuale su una
 * base. Utile per i tempi di servizio (es. 5000ms +/- 50%).
 *
 * @param base Valore base
 * @param variance_percent Percentuale di varianza (+/-)
 * @param generator Puntatore alla funzione di generazione (es. rand)
 *
 * @return double Valore calcolato
 */
double random_variance(double base, double variance_percent,
                       int (*generator)(void));
#endif