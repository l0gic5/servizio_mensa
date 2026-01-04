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
  }

  else {
    LOG_WARN("CONFIG", "chiave sconosciuta alla riga %d: %s", line_num, key);
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
    fclose(file);
  }

  return result;
}