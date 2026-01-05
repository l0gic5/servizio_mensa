#include "common/config.h"

/**
 * @brief Trims leading and trailing whitespace from a string
 *
 * @param str The string to trim
 * @return char* Pointer to the trimmed string
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
 * @brief Applies a configuration parameter to the Config struct
 *
 * @param config Pointer to the Config struct
 * @param key The configuration key
 * @param value The configuration value
 * @param line_num The line number in the config file (for logging)
 */
static void apply_config_parameter(Config *config, const char *key,
                                   const char *value, int line_num) {
  ///////////////////////////
  //  Simulazione e Utenti //
  ///////////////////////////

  if (strcmp(key, "SIM_DURATION") == 0) {
    config->sim_duration = atoi(value);
  } else if (strcmp(key, "N_NANO_SECS") == 0) {
    config->n_nano_secs = atoi(value);
  } else if (strcmp(key, "NOF_USERS") == 0) {
    config->n_users = atoi(value);
  } else if (strcmp(key, "NOF_WORKERS") == 0) {
    config->n_workers = atoi(value);
  } else if (strcmp(key, "NOF_TABLE_SEATS") == 0) {
    config->table_seats = atoi(value);
  } else if (strcmp(key, "OVERLOAD_THRESHOLD") == 0) {
    config->overload_threshold = atoi(value);
  } else if (strcmp(key, "MINUTI_SERVIZIO_GIORNALIERO") == 0) {
    config->minuti_servizio_giornaliero = atoi(value);
  }

  ////////////////////////
  //  Tempi di servizio //
  ////////////////////////

  else if (strcmp(key, "AVG_SRVC_PRIMI") == 0) {
    config->avg_service_primi = atoi(value);
  } else if (strcmp(key, "AVG_SRVC_MAIN_COURSE") == 0) {
    config->avg_service_secondi = atoi(value);
  } else if (strcmp(key, "AVG_SRVC_COFFEE") == 0) {
    config->avg_service_coffee = atoi(value);
  } else if (strcmp(key, "AVG_SRVC_CASSA") == 0) {
    config->avg_service_cassa = atoi(value);
  }

  ////////////////////////////////////
  //  Postazioni Fisiche Operatori  //
  ////////////////////////////////////

  else if (strcmp(key, "WORKSTATIONS_PRIMI") == 0) {
    config->workstations_primi = atoi(value);
  } else if (strcmp(key, "WORKSTATIONS_SECONDI") == 0) {
    config->workstations_secondi = atoi(value);
  } else if (strcmp(key, "WORKSTATIONS_COFFEE") == 0) {
    config->workstations_coffee = atoi(value);
  } else if (strcmp(key, "WORKSTATIONS_CASSA") == 0) {
    config->workstations_cassa = atoi(value);
  }

  ///////////////////////
  //  Pause Operatori  //
  ///////////////////////

  else if (strcmp(key, "MAX_PAUSES_PER_DAY") == 0) {
    config->max_pauses_per_day = atoi(value);
  } else if (strcmp(key, "PAUSE_DURATION_NS") == 0) {
    config->pause_duration_ns = atoi(value);
  } else if (strcmp(key, "PAUSE_PROBABILITY_PERCENT") == 0) {
    config->pause_probability_percent = atoi(value);
  }

  /////////////////////
  //  Code Stazioni  //
  /////////////////////

  else if (strcmp(key, "NOF_WK_SEATS_PRIMI") == 0) {
    config->seats_primi = atoi(value);
  } else if (strcmp(key, "NOF_WK_SEATS_SECONDI") == 0) {
    config->seats_secondi = atoi(value);
  } else if (strcmp(key, "NOF_WK_SEATS_COFFEE") == 0) {
    config->seats_coffee = atoi(value);
  } else if (strcmp(key, "NOF_WK_SEATS_CASSA") == 0) {
    config->seats_cassa = atoi(value);
  }

  //////////////
  //  Prezzi  //
  //////////////

  else if (strcmp(key, "PRICE_PRIMI") == 0) {
    config->price_primi = atoi(value);
  } else if (strcmp(key, "PRICE_SECONDI") == 0) {
    config->price_secondi = atoi(value);
  } else if (strcmp(key, "PRICE_COFFEE") == 0) {
    config->price_coffee = atoi(value);
  }

  ///////////////////
  //  Rifornimenti //
  ///////////////////

  else if (strcmp(key, "AVG_REFILL_PRIMI") == 0) {
    config->avg_refill_primi = atoi(value);
  } else if (strcmp(key, "AVG_REFILL_SECONDI") == 0) {
    config->avg_refill_secondi = atoi(value);
  } else if (strcmp(key, "MAX_PORZIONI_PRIMI") == 0) {
    config->max_porzioni_primi = atoi(value);
  } else if (strcmp(key, "MAX_PORZIONI_SECONDI") == 0) {
    config->max_porzioni_secondi = atoi(value);
  } else if (strcmp(key, "VARIABILITY_PRIMI") == 0) {
    config->variability_primi = atoi(value);
  } else if (strcmp(key, "VARIABILITY_SECONDI") == 0) {
    config->variability_secondi = atoi(value);
  } else if (strcmp(key, "VARIABILITY_COFFEE") == 0) {
    config->variability_coffee = atoi(value);
  } else if (strcmp(key, "VARIABILITY_CASSA") == 0) {
    config->variability_cassa = atoi(value);
  }

  else {
    LOG_WARN("CONFIG", "chiave sconosciuta alla riga %d: %s", line_num, key);
  }
}

/**
 * @brief Imposta i valori di default per i parametri di configurazione
 * opzionali.
 *
 * Questa funzione viene chiamata dopo il parsing del file. Se alcune
 * variabili cruciali sono rimaste a 0 (non specificate nel file), vengono
 * assegnati i valori della "CONFIGURAZIONE STANDARD".
 *
 * @param config Puntatore alla struttura Config da completare.
 */
static void set_default_values(Config *config) {
  // Simulazione e Utenti
  if (config->sim_duration == 0) {
    config->sim_duration = 30;
  }
  if (config->n_nano_secs == 0) {
    config->n_nano_secs = 100000000; // 100ms
  }
  if (config->minuti_servizio_giornaliero == 0) {
    config->minuti_servizio_giornaliero = 120; // 2 ore
  }
  if (config->n_users == 0) {
    config->n_users = 40;
  }
  if (config->n_workers == 0) {
    config->n_workers = 6;
  }
  if (config->table_seats == 0) {
    config->table_seats = 30;
  }
  if (config->overload_threshold == 0) {
    config->overload_threshold = 10;
  }

  // Tempi di servizio
  if (config->avg_service_primi == 0) {
    config->avg_service_primi = 5000;
  }
  if (config->avg_service_secondi == 0) {
    config->avg_service_secondi = 6000;
  }
  if (config->avg_service_coffee == 0) {
    config->avg_service_coffee = 2000;
  }
  if (config->avg_service_cassa == 0) {
    config->avg_service_cassa = 3000;
  }

  // Postazioni Fisiche Operatori
  if (config->workstations_primi == 0) {
    config->workstations_primi = 5;
  }
  if (config->workstations_secondi == 0) {
    config->workstations_secondi = 5;
  }
  if (config->workstations_coffee == 0) {
    config->workstations_coffee = 5;
  }
  if (config->workstations_cassa == 0) {
    config->workstations_cassa = 1;
  }

  // Pause
  if (config->max_pauses_per_day == 0) {
    config->max_pauses_per_day = 3;
  }
  if (config->pause_duration_ns == 0) {
    config->pause_duration_ns = 500000000; // 0.5 sec
  }
  if (config->pause_probability_percent == 0) {
    config->pause_probability_percent = 10; // 10%
  }

  // Capacità Code (Utenti)
  if (config->seats_primi == 0) {
    config->seats_primi = 10;
  }
  if (config->seats_secondi == 0) {
    config->seats_secondi = 10;
  }
  if (config->seats_coffee == 0) {
    config->seats_coffee = 15;
  }
  if (config->seats_cassa == 0) {
    config->seats_cassa = 15;
  }

  // Prezzi
  if (config->price_primi == 0) {
    config->price_primi = 5;
  }
  if (config->price_secondi == 0) {
    config->price_secondi = 8;
  }
  if (config->price_coffee == 0) {
    config->price_coffee = 1;
  }

  // Rifornimenti
  if (config->avg_refill_primi == 0) {
    config->avg_refill_primi = 50000;
  }
  if (config->avg_refill_secondi == 0) {
    config->avg_refill_secondi = 50000;
  }
  if (config->max_porzioni_primi == 0) {
    config->max_porzioni_primi = 100;
  }
  if (config->max_porzioni_secondi == 0) {
    config->max_porzioni_secondi = 100;
  }

  // Variability
  if (config->variability_primi == 0) {
    config->variability_primi = 50;
  }
  if (config->variability_secondi == 0) {
    config->variability_secondi = 50;
  }
  if (config->variability_coffee == 0) {
    config->variability_coffee = 80;
  }
  if (config->variability_cassa == 0) {
    config->variability_cassa = 10;
  }
}

int parse_config(const char *filename, Config *config) {
  FILE *file = fopen(filename, "r");
  int result = 0;

  if (!file) {
    LOG_ERR("CONFIG", "impossibile aprire il file di configurazione: %s",
            filename);
    result = -1;
  } else {
    char line[MAX_LINE_LENGTH];
    int line_num = 0;

    memset(config, 0, sizeof(Config));

    while (fgets(line, sizeof(line), file)) {
      line_num++;

      if ((line[0] != '#' && line[0] != '\n' && line[0] != '\0')) {
        char *separator = strchr(line, '=');

        if (separator) {
          *separator = '\0';
          char *key = trim(line);
          char *value = trim(separator + 1);

          if (strlen(key) > 0 && strlen(value) > 0) {
            apply_config_parameter(config, key, value, line_num);
          } else {
            LOG_WARN("CONFIG", "riga %d non valida: %s=%s", line_num, key,
                     value);
          }
        }
        // else => se non c'è separatore
        // ignoro la riga
      }
    }

    // DEFAULT PER PARAMETRI NON SPECIFICATI
    set_default_values(config);

    fclose(file);
  }

  return result;
}