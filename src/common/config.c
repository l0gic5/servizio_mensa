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
  } else if (strcmp(key, "AVG_USER_W_TICKET") == 0) {
    config->avg_user_w_ticket = atoi(value);
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

  // tickets
  else if (strcmp(key, "TICKET_READER_CAPACITY") == 0) {
    config->ticket_reader_capacity = atoi(value);
  } else if (strcmp(key, "TICKET_READER_TIMEOUT_NS") == 0) {
    config->ticket_reader_timeout_ns = atoi(value);
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
    config->price_primi = atof(value);
  } else if (strcmp(key, "PRICE_SECONDI") == 0) {
    config->price_secondi = atof(value);
  } else if (strcmp(key, "PRICE_CAFFE") == 0) {
    config->price_caffe = atof(value);
  } else if (strcmp(key, "TICKET_DISCOUNT_PERCENT") == 0) {
    config->ticket_discount_percent = atof(value);
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
  } else if (strcmp(key, "DAY_END_BARRIER_WAIT_SEC") == 0) {
    config->day_end_barrier_wait_sec = atoi(value);
  }

  ////////////////////////////////////////////
  //  6) COMPORTAMENTO E PREFERENZE UTENTI  //
  ////////////////////////////////////////////

  // comportamento
  else if (strcmp(key, "USER_QUEUE_TIMEOUT_SEC") == 0) {
    config->user_queue_timeout_sec = atoi(value);
  } else if (strcmp(key, "USER_MEAL_DURATION_NS") == 0) {
    config->user_meal_duration_ns = atoi(value);
  } else if (strcmp(key, "USER_COFFEE_DURATION_NS") == 0) {
    config->user_coffee_duration_ns = atoi(value);
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
  } else if (strcmp(key, "REFILL_VARIANCE_PERCENT") == 0) {
    config->refill_variance_percent = atoi(value);
  }

  else if (strcmp(key, "AVG_REFILL_PRIMI") == 0) {
    config->avg_refill_primi = atoi(value);
  } else if (strcmp(key, "AVG_REFILL_SECONDI") == 0) {
    config->avg_refill_secondi = atoi(value);
  } else if (strcmp(key, "AVG_REFILL_CAFFE") == 0) {
    config->avg_refill_caffe = atoi(value);
  }

  else if (strcmp(key, "MAX_PORZIONI_PRIMI") == 0) {
    config->max_porzioni_primi = atoi(value);
  } else if (strcmp(key, "MAX_PORZIONI_SECONDI") == 0) {
    config->max_porzioni_secondi = atoi(value);
  } else if (strcmp(key, "MAX_PORZIONI_CAFFE") == 0) {
    config->max_porzioni_caffe = atoi(value);
  }

  ////////////////
  //  8) TOOLS  //
  ////////////////

  else if (strcmp(key, "DEFAULT_SCIOPERO_STOP_DURATION") == 0) {
    config->default_sciopero_stop_duration = atoi(value);
  }

  else if (strcmp(key, "EXPORT_FOLDER_PATH") == 0) {
    strncpy(config->export_folder_path, value,
            sizeof(config->export_folder_path) - 1);
  } else if (strcmp(key, "EXPORT_DAILY_REPORTS_CSV") == 0) {
    config->export_daily_reports_csv =
        (strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0);
  } else if (strcmp(key, "CREATE_DAILY_SINGLE_FILES") == 0) {
    config->create_daily_single_files =
        (strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0);
  } else if (strcmp(key, "DAILY_REPORTS_FILENAME_CSV") == 0) {
    strncpy(config->daily_reports_filename_csv, value,
            sizeof(config->daily_reports_filename_csv) - 1);
  } else if (strcmp(key, "EXPORT_FINAL_STATS_CSV") == 0) {
    config->export_final_stats_csv =
        (strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0);
  } else if (strcmp(key, "FINAL_STATS_FILENAME_CSV") == 0) {
    strncpy(config->final_stats_filename_csv, value,
            sizeof(config->final_stats_filename_csv) - 1);
  }

  else {
    LOG_WARN("CONFIG", "Chiave sconosciuta riga %d: %s", line_num, key);
  }
}

/**
 * @brief Sets default values for optional parameters.
 */
static void set_default_values(Config *config) {
  // 1) SIMULAZIONE GLOBALE
  config->simulation_duration_days = 30;
  config->n_nanosecs_as_minute = 100000000;
  config->daily_service_minutes = 120;
  config->system_startup_delay_sec = 2;
  config->overload_threshold = 50;

  // 2) POPOLAZIONE
  config->nof_users = 50;
  config->nof_workers = 6;
  config->nof_table_seats = 40;
  config->avg_user_w_ticket = 80;

  // 3) STAZIONI
  config->queue_capacity_primi = 15;
  config->queue_capacity_secondi = 15;
  config->queue_capacity_caffe = 20;
  config->queue_capacity_cassa = 20;

  config->ticket_reader_capacity = 3;
  config->ticket_reader_timeout_ns = 5000000;

  config->workstations_primi = 2;
  config->workstations_secondi = 2;
  config->workstations_caffe = 2;
  config->workstations_cassa = 1;

  // 4) METRICHE
  config->avg_service_primi = 100000000;
  config->avg_service_secondi = 120000000;
  config->avg_service_caffe = 50000000;
  config->avg_service_cassa = 60000000;

  config->variability_primi = 30;
  config->variability_secondi = 30;
  config->variability_caffe = 20;
  config->variability_cassa = 20;

  config->price_primi = 5.5;
  config->price_secondi = 8.2;
  config->price_caffe = 1.2;
  config->ticket_discount_percent = 25.0;

  // 5) OPERATORI
  config->max_pauses_per_day = 2;
  config->pause_duration_ns = 300000000;
  config->pause_probability_percent = 10;
  config->day_end_barrier_wait_sec = 30;

  // 6) UTENTI
  config->user_queue_timeout_sec = 5;
  config->user_meal_duration_ns = 2000000000;
  config->user_coffee_duration_ns = 10000000;
  config->user_max_arrival_delay_us = 9600000;

  config->probability_user_wants_primo = 60;
  config->probability_user_wants_secondo = 70;
  config->probability_user_wants_caffe = 20;

  config->user_budget_min = 10;
  config->user_budget_max = 50;
  config->user_min_daily_salary = 5;
  config->user_max_daily_salary = 15;

  // 7) LOGISTICA
  config->refill_interval_minutes = 10;
  config->refill_variance_percent = 20;

  config->avg_refill_primi = 20;
  config->avg_refill_secondi = 20;
  config->avg_refill_caffe = 500;

  config->max_porzioni_primi = 50;
  config->max_porzioni_secondi = 50;
  config->max_porzioni_caffe = 1000;

  // 8) TOOLS
  config->default_sciopero_stop_duration = 60;

  strncpy(config->export_folder_path, "reports/",
          sizeof(config->export_folder_path) - 1);

  config->export_daily_reports_csv = true;
  config->create_daily_single_files = false;
  strncpy(config->daily_reports_filename_csv, "days/daily_report",
          sizeof(config->daily_reports_filename_csv) - 1);

  config->export_final_stats_csv = true;
  strncpy(config->final_stats_filename_csv, "final_stats",
          sizeof(config->final_stats_filename_csv) - 1);
}

int parse_config(const char *filename, Config *config) {
  FILE *file = fopen(filename, "r");
  int result = 0;

  memset(config, 0, sizeof(Config));

  set_default_values(config);

  if (!file) {
    LOG_ERR("CONFIG", "Impossibile aprire file config (uso default): %s",
            filename);
    // result = -1;
  } else {
    char line[MAX_LINE_LENGTH];
    int line_num = 0;

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

    fclose(file);
  }
  return result;
}