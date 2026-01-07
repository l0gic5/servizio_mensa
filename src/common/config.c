#include "common/config.h"

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

double random_range(double min, double max, int (*generator)(void)) {
  long range = (long)(max - min + 1);

  long random_val = generator();

  return min + (double)(random_val % range);
}

bool random_probability(int percent, int (*generator)(void)) {
  return (generator() % 100) < percent;
}

double random_variance(double base, double variance_percent,
                       int (*generator)(void)) {
  double range = (base * variance_percent) / 100.0;
  double min = base - range;
  double max = base + range;

  return random_range(min, max, generator);
}

/**
 * @brief Applies a configuration parameter to the Config struct.
 * Mappa le chiavi "LEGACY" (dal PDF) alle variabili "NUOVE" (Autoesplicative).
 */
static void apply_config_parameter(Config *config, const char *key,
                                   const char *value, int line_num) {

  ///////////////////////////////////////////////
  //  1) SIMULAZIONE GLOBALE (Tempo e Limiti)  //
  ///////////////////////////////////////////////

  if (strcmp(key, "SIM_DURATION") == 0) {
    config->simulation_duration_days = atoi(value);
  } else if (strcmp(key, "N_NANO_SECS") == 0) {
    config->n_nanosecs_as_minute = atoi(value);
  } else if (strcmp(key, "DAILY_SERVICE_MINUTES") == 0) {
    config->daily_service_minutes = atoi(value);
  } else if (strcmp(key, "SYSTEM_STARTUP_DELAY_SEC") == 0) {
    config->system_startup_delay_sec = atoi(value);
  } else if (strcmp(key, "OVERLOAD_THRESHOLD") == 0) {
    config->overload_threshold = atoi(value);
  } else if (strcmp(key, "CASSA_POSITION") == 0) {
    if (strcmp(value, "INGRESSO") == 0)
      config->cassa_position = INGRESSO;
    else if (strcmp(value, "USCITA") == 0)
      config->cassa_position = USCITA;
    else
      LOG_WARN("CONFIG", "Invalid CASSA_POSITION at line %d: %s", line_num,
               value);
  }

  ////////////////////////////////////////
  //  2) POPOLAZIONE E RISORSE FISICHE  //
  ////////////////////////////////////////

  else if (strcmp(key, "NOF_USERS") == 0) {
    config->nof_users = atoi(value);
  } else if (strcmp(key, "NOF_WORKERS") == 0) {
    config->nof_workers = atoi(value);
  } else if (strcmp(key, "NOF_TABLE_SEATS") == 0) {
    config->nof_table_seats = atoi(value);
  }

  ///////////////////////////////////////////////////
  //  3) CONFIGURAZIONE STAZIONI (Code e Banconi)  //
  ///////////////////////////////////////////////////

  // capacità code
  else if (strcmp(key, "NOF_WK_SEATS_PRIMI") == 0) {
    config->queue_capacity_primi = atoi(value);
  } else if (strcmp(key, "NOF_WK_SEATS_SECONDI") == 0) {
    config->queue_capacity_secondi = atoi(value);
  } else if (strcmp(key, "NOF_WK_SEATS_CAFFE") == 0) {
    config->queue_capacity_caffe = atoi(value);
  } else if (strcmp(key, "NOF_WK_SEATS_CASSA") == 0) {
    config->queue_capacity_cassa = atoi(value);
  }
  // postazioni fisiche
  else if (strcmp(key, "WORKSTATIONS_PRIMI") == 0) {
    config->workstations_primi = atoi(value);
  } else if (strcmp(key, "WORKSTATIONS_SECONDI") == 0) {
    config->workstations_secondi = atoi(value);
  } else if (strcmp(key, "WORKSTATIONS_CAFFE") == 0) {
    config->workstations_caffe = atoi(value);
  } else if (strcmp(key, "WORKSTATIONS_CASSA") == 0) {
    config->workstations_cassa = atoi(value);
  }

  ////////////////////////////////////////////////////////////
  //  4) METRICHE DI SERVIZIO (Tempi, Variabilità, Prezzi)  //
  ////////////////////////////////////////////////////////////

  // tempi medi
  else if (strcmp(key, "AVG_SRVC_PRIMI") == 0) {
    config->avg_service_primi = atoi(value);
  } else if (strcmp(key, "AVG_SRVC_SECONDI") == 0) {
    config->avg_service_secondi = atoi(value);
  } else if (strcmp(key, "AVG_SRVC_CAFFE") == 0) {
    config->avg_service_caffe = atoi(value);
  } else if (strcmp(key, "AVG_SRVC_CASSA") == 0) {
    config->avg_service_cassa = atoi(value);
  }
  // variabilità
  else if (strcmp(key, "VARIABILITY_PRIMI") == 0) {
    config->variability_primi = atoi(value);
  } else if (strcmp(key, "VARIABILITY_SECONDI") == 0) {
    config->variability_secondi = atoi(value);
  } else if (strcmp(key, "VARIABILITY_CAFFE") == 0) {
    config->variability_caffe = atoi(value);
  } else if (strcmp(key, "VARIABILITY_CASSA") == 0) {
    config->variability_cassa = atoi(value);
  }
  // prezzi
  else if (strcmp(key, "PRICE_PRIMI") == 0) {
    config->price_primi = atoi(value);
  } else if (strcmp(key, "PRICE_SECONDI") == 0) {
    config->price_secondi = atoi(value);
  } else if (strcmp(key, "PRICE_CAFFE") == 0) {
    config->price_caffe = atoi(value);
  }

  //////////////////////////////////////////
  //  5) COMPORTAMENTO OPERATORI (Pause)  //
  //////////////////////////////////////////

  else if (strcmp(key, "MAX_PAUSES_PER_DAY") == 0) {
    config->max_pauses_per_day = atoi(value);
  } else if (strcmp(key, "PAUSE_DURATION_NS") == 0) {
    config->pause_duration_ns = atoi(value);
  } else if (strcmp(key, "PAUSE_PROBABILITY_PERCENT") == 0) {
    config->pause_probability_percent = atoi(value);
  }
  ////////////////////////////////////////////
  //  6) COMPORTAMENTO E PREFERENZE UTENTI  //
  ////////////////////////////////////////////

  // comportamento
  else if (strcmp(key, "USER_QUEUE_TIMEOUT_SEC") == 0) {
    config->user_queue_timeout_sec = atoi(value);
  } else if (strcmp(key, "USER_MEAL_DURATION_NS") == 0) {
    config->user_meal_duration_ns = atoi(value);
  } else if (strcmp(key, "USER_MAX_ARRIVAL_DELAY_US") == 0) {
    config->user_max_arrival_delay_us = atoi(value);
  }
  // preferenze
  else if (strcmp(key, "PROBABILITY_USER_WANTS_PRIMO") == 0) {
    config->probability_user_wants_primo = atoi(value);
  } else if (strcmp(key, "PROBABILITY_USER_WANTS_SECONDO") == 0) {
    config->probability_user_wants_secondo = atoi(value);
  } else if (strcmp(key, "PROBABILITY_USER_WANTS_CAFFE") == 0) {
    config->probability_user_wants_caffe = atoi(value);
  }

  // budget
  else if (strcmp(key, "USER_BUDGET_MIN") == 0) {
    config->user_budget_min = atoi(value);
  } else if (strcmp(key, "USER_BUDGET_MAX") == 0) {
    config->user_budget_max = atoi(value);
  } else if (strcmp(key, "USER_MIN_DAILY_SALARY") == 0) {
    config->user_min_daily_salary = atoi(value);
  } else if (strcmp(key, "USER_MAX_DAILY_SALARY") == 0) {
    config->user_max_daily_salary = atoi(value);
  }

  ///////////////////////////////////
  //  7) LOGISTICA (Rifornimenti)  //
  ///////////////////////////////////

  else if (strcmp(key, "REFILL_INTERVAL_MINUTES") == 0) {
    config->refill_interval_minutes = atoi(value);
  } else if (strcmp(key, "AVG_REFILL_PRIMI") == 0) {
    config->avg_refill_primi = atoi(value);
  } else if (strcmp(key, "AVG_REFILL_SECONDI") == 0) {
    config->avg_refill_secondi = atoi(value);
  } else if (strcmp(key, "MAX_PORZIONI_PRIMI") == 0) {
    config->max_porzioni_primi = atoi(value);
  } else if (strcmp(key, "MAX_PORZIONI_SECONDI") == 0) {
    config->max_porzioni_secondi = atoi(value);
  } else if (strcmp(key, "MAX_PORZIONI_CAFFE") == 0) {
    config->max_porzioni_caffe = atoi(value);
  }

  else {
    LOG_WARN("CONFIG", "Chiave sconosciuta riga %d: %s", line_num, key);
  }
}

/**
 * @brief Sets default values for optional parameters.
 */
static void set_default_values(Config *config) {
  ///////////////////////////////////////////////
  //  1) SIMULAZIONE GLOBALE (Tempo e Limiti)  //
  ///////////////////////////////////////////////

  if (config->simulation_duration_days == 0) {
    config->simulation_duration_days = 15;
  }
  if (config->n_nanosecs_as_minute == 0) {
    config->n_nanosecs_as_minute = 100000000;
  }
  if (config->daily_service_minutes == 0) {
    config->daily_service_minutes = 120;
  }
  if (config->system_startup_delay_sec == 0) {
    config->system_startup_delay_sec = 2;
  }
  if (config->overload_threshold == 0) {
    config->overload_threshold = 50;
  }
  // cassa position default is INGRESSO (enum 0)

  ////////////////////////////////////////
  //  2) POPOLAZIONE E RISORSE FISICHE  //
  ////////////////////////////////////////
  if (config->nof_users == 0) {
    config->nof_users = 50;
  }
  if (config->nof_workers == 0) {
    config->nof_workers = 6;
  }
  if (config->nof_table_seats == 0) {
    config->nof_table_seats = 40;
  }

  ///////////////////////////////////////////////////
  //  3) CONFIGURAZIONE STAZIONI (Code e Banconi)  //
  ///////////////////////////////////////////////////

  // capacità code
  if (config->queue_capacity_primi == 0) {
    config->queue_capacity_primi = 15;
  }
  if (config->queue_capacity_secondi == 0) {
    config->queue_capacity_secondi = 15;
  }
  if (config->queue_capacity_caffe == 0) {
    config->queue_capacity_caffe = 20;
  }
  if (config->queue_capacity_cassa == 0) {
    config->queue_capacity_cassa = 20;
  }
  // postazioni fisiche
  if (config->workstations_primi == 0) {
    config->workstations_primi = 2;
  }
  if (config->workstations_secondi == 0) {
    config->workstations_secondi = 2;
  }
  if (config->workstations_caffe == 0) {
    config->workstations_caffe = 2;
  }
  if (config->workstations_cassa == 0) {
    config->workstations_cassa = 1;
  }

  ////////////////////////////////////////////////////////////
  //  4) METRICHE DI SERVIZIO (Tempi, Variabilità, Prezzi)  //
  ////////////////////////////////////////////////////////////

  // tempi medi
  if (config->avg_service_primi == 0) {
    config->avg_service_primi = 100000000;
  }
  if (config->avg_service_secondi == 0) {
    config->avg_service_secondi = 120000000;
  }
  if (config->avg_service_caffe == 0) {
    config->avg_service_caffe = 50000000;
  }
  if (config->avg_service_cassa == 0) {
    config->avg_service_cassa = 60000000;
  }
  // variabilità
  if (config->variability_primi == 0) {
    config->variability_primi = 30;
  }
  if (config->variability_secondi == 0) {
    config->variability_secondi = 30;
  }
  if (config->variability_caffe == 0) {
    config->variability_caffe = 20;
  }
  if (config->variability_cassa == 0) {
    config->variability_cassa = 20;
  }
  // prezzi
  if (config->price_primi == 0) {
    config->price_primi = 5.5;
  }
  if (config->price_secondi == 0) {
    config->price_secondi = 8.2;
  }
  if (config->price_caffe == 0) {
    config->price_caffe = 1.2;
  }

  //////////////////////////////////////////
  //  5) COMPORTAMENTO OPERATORI (Pause)  //
  //////////////////////////////////////////

  if (config->max_pauses_per_day == 0) {
    config->max_pauses_per_day = 2;
  }
  if (config->pause_duration_ns == 0) {
    config->pause_duration_ns = 300000000;
  }
  if (config->pause_probability_percent == 0) {
    config->pause_probability_percent = 10;
  }

  ////////////////////////////////////////////
  //  6) COMPORTAMENTO E PREFERENZE UTENTI  //
  ////////////////////////////////////////////

  // comportamento
  if (config->user_queue_timeout_sec == 0) {
    config->user_queue_timeout_sec = 2;
  }
  if (config->user_meal_duration_ns == 0) {
    config->user_meal_duration_ns = 2000000000;
  }
  if (config->user_max_arrival_delay_us == 0) {
    config->user_max_arrival_delay_us = 5000000;
  }
  // preferenze
  if (config->probability_user_wants_primo == 0) {
    config->probability_user_wants_primo = 60;
  }
  if (config->probability_user_wants_secondo == 0) {
    config->probability_user_wants_secondo = 70;
  }
  if (config->probability_user_wants_caffe == 0) {
    config->probability_user_wants_caffe = 20;
  }

  // budget
  if (config->user_budget_min == 0) {
    config->user_budget_min = 10;
  }
  if (config->user_budget_max == 0) {
    config->user_budget_max = 50;
  }
  if (config->user_min_daily_salary == 0) {
    config->user_min_daily_salary = 5;
  }
  if (config->user_max_daily_salary == 0) {
    config->user_max_daily_salary = 15;
  }

  ///////////////////////////////////
  //  7) LOGISTICA (Rifornimenti)  //
  ///////////////////////////////////

  if (config->refill_interval_minutes == 0) {
    config->refill_interval_minutes = 10;
  }

  if (config->avg_refill_primi == 0) {
    config->avg_refill_primi = 40;
  }
  if (config->avg_refill_secondi == 0) {
    config->avg_refill_secondi = 40;
  }

  if (config->max_porzioni_primi == 0) {
    config->max_porzioni_primi = 50;
  }
  if (config->max_porzioni_secondi == 0) {
    config->max_porzioni_secondi = 50;
  }
  if (config->max_porzioni_caffe == 0) {
    config->max_porzioni_caffe = 1000;
  }
}

int parse_config(const char *filename, Config *config) {
  FILE *file = fopen(filename, "r");
  int result = 0;

  if (!file) {
    LOG_ERR("CONFIG", "Impossibile aprire file config: %s", filename);
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
            LOG_WARN("CONFIG", "Riga %d non valida: %s=%s", line_num, key,
                     value);
          }
        }
      }
    }
    set_default_values(config);
    fclose(file);
  }
  return result;
}