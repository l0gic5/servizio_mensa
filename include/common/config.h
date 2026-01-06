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
  int queue_capacity_coffee;  // NOF_WK_SEATS_COFFEE
  int queue_capacity_cassa;   // NOF_WK_SEATS_CASSA

  // postazioni fisiche (numero operatori contemporanei)
  int workstations_primi;   // WORKSTATIONS_PRIMI
  int workstations_secondi; // WORKSTATIONS_SECONDI
  int workstations_coffee;  // WORKSTATIONS_COFFEE
  int workstations_cassa;   // WORKSTATIONS_CASSA

  ////////////////////////////////////////////////////////////
  //  4) METRICHE DI SERVIZIO (Tempi, Variabilità, Prezzi)  //
  ////////////////////////////////////////////////////////////

  // tempi medi base
  int avg_service_primi;   // AVG_SRVC_PRIMI
  int avg_service_secondi; // AVG_SRVC_SECONDI
  int avg_service_coffee;  // AVG_SRVC_COFFEE
  int avg_service_cassa;   // AVG_SRVC_CASSA

  // variabilità (% +/-)
  int variability_primi;   // VARIABILITY_PRIMI
  int variability_secondi; // VARIABILITY_SECONDI
  int variability_coffee;  // VARIABILITY_COFFEE
  int variability_cassa;   // VARIABILITY_CASSA

  // prezzi
  int price_primi;   // PRICE_PRIMI
  int price_secondi; // PRICE_SECONDI
  int price_coffee;  // PRICE_COFFEE

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
  int probability_user_wants_coffee;  // PROBABILITY_USER_WANTS_COFFEE

  ///////////////////////////////////
  //  7) LOGISTICA (Rifornimenti)  //
  ///////////////////////////////////

  int avg_refill_primi;     // AVG_REFILL_PRIMI
  int avg_refill_secondi;   // AVG_REFILL_SECONDI
  int max_porzioni_primi;   // MAX_PORZIONI_PRIMI
  int max_porzioni_secondi; // MAX_PORZIONI_SECONDI

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

#endif