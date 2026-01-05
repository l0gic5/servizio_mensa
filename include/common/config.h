#ifndef CONFIG_H
#define CONFIG_H

#include "common/logger.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 512

typedef struct config {
  // OVERLOAD_THRESHOLD
  int overload_threshold;

  // MINUTI_SERVIZIO_GIORNALIERO
  int minuti_servizio_giornaliero;

  // N_NANO_SECS
  int n_nano_secs;

  // SIM_DURATION
  int sim_duration;
  // NOF_USERS
  int n_users;
  // NOF_WORKERS
  int n_workers;
  // NOF_TABLE_SEATS
  int table_seats;

  // tempi di servizio medi (in secondi o nanosecondi simulati)
  // AVG_SRVC_PRIMI
  int avg_service_primi;
  // AVG_SRVC_MAIN_COURSE
  int avg_service_secondi;
  // AVG_SRVC_COFFEE
  int avg_service_coffee;
  // AVG_SRVC_CASSA
  int avg_service_cassa;

  // postazioni fisiche operatori
  // WORKSTATIONS_PRIMI
  int workstations_primi;
  // WORKSTATIONS_SECONDI
  int workstations_secondi;
  // WORKSTATIONS_COFFEE
  int workstations_coffee;
  // WORKSTATIONS_CASSA
  int workstations_cassa;

  // pause operatori
  // MAX_PAUSES_PER_DAY
  int max_pauses_per_day;
  // PAUSE_DURATION_NS
  int pause_duration_ns;
  // PAUSE_PROBABILITY_PERCENT
  int pause_probability_percent;

  // capacità code stazioni
  // NOF_WK_SEATS_PRIMI
  int seats_primi;
  // NOF_WK_SEATS_SECONDI
  int seats_secondi;
  // NOF_WK_SEATS_COFFEE
  int seats_coffee;
  // NOF_WK_SEATS_CASSA
  int seats_cassa;

  // prezzi
  // PRICE_PRIMI
  int price_primi;
  // PRICE_SECONDI
  int price_secondi;
  // PRICE_COFFEE
  int price_coffee;

  // rifornimenti
  // AVG_REFILL_PRIMI
  int avg_refill_primi;
  // AVG_REFILL_SECONDI
  int avg_refill_secondi;
  // MAX_PORZIONI_PRIMI
  int max_porzioni_primi;
  // MAX_PORZIONI_SECONDI
  int max_porzioni_secondi;

  // VARIABILITY_PRIMI
  int variability_primi;
  // VARIABILITY_SECONDI
  int variability_secondi;
  // VARIABILITY_COFFEE
  int variability_coffee;
  // VARIABILITY_CASSA
  int variability_cassa;
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