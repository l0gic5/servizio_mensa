#include "common/names.h"

static char **g_names_m = NULL;
static int g_count_m = 0;

static char **g_names_f = NULL;
static int g_count_f = 0;

static char **g_surnames = NULL;
static int g_count_s = 0;

/**
 * @brief Rimuove il carattere newline finale da una stringa, se presente.
 *
 * @param str La stringa da modificare
 */
static void trim_newline(char *str) {
  size_t len = strlen(str);
  if (len > 0 && str[len - 1] == '\n') {
    str[len - 1] = '\0';
  }
}

/**
 * @brief Carica un file di testo in un array di stringhe, una per riga.
 *
 * @param path Il percorso del file da caricare
 * @param list_out Puntatore all'array di stringhe risultante
 * @param count_out Numero di stringhe caricate
 *
 * @return int 0 se successo, -1 se errore
 */
static int load_list(const char *path, char ***list_out, int *count_out) {
  FILE *f = fopen(path, "r");
  if (!f) {
    LOG_WARN("NAMES", "Impossibile aprire %s.", path);
    return -1;
  }

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  int count = 0;
  int capacity = 100;

  char **list = malloc((unsigned long)capacity * sizeof(char *));

  while ((read = getline(&line, &len, f)) != -1) {
    if (count >= capacity) {
      capacity *= 2;
      list = realloc(list, (unsigned long)capacity * sizeof(char *));
    }
    trim_newline(line);

    if (strlen(line) > 0) {
      list[count] = strdup(line);
      count++;
    }
  }

  free(line);
  fclose(f);

  *list_out = list;
  *count_out = count;
  return 0;
}

/**
 * @brief Inizializza le liste di nomi e cognomi caricandole dai file.
 * Deve essere chiamata una volta all'inizio del main di ogni processo.
 *
 * @return int 0 se successo, -1 se errore
 */
int names_init(void) {
  if (!g_names_m) {
    load_list(PATH_NOMI_M, &g_names_m, &g_count_m);
  }
  if (!g_names_f) {
    load_list(PATH_NOMI_F, &g_names_f, &g_count_f);
  }
  if (!g_surnames) {
    load_list(PATH_COGNOMI, &g_surnames, &g_count_s);
  }
  return 0;
}

/**
 * @brief Libera la memoria allocata per le liste di nomi.
 */
void names_destroy(void) {
  if (g_names_m) {
    for (int i = 0; i < g_count_m; i++) {
      free(g_names_m[i]);
    }
    free(g_names_m);
  }
  if (g_names_f) {
    for (int i = 0; i < g_count_f; i++) {
      free(g_names_f[i]);
    }
    free(g_names_f);
  }
  if (g_surnames) {
    for (int i = 0; i < g_count_s; i++) {
      free(g_surnames[i]);
    }
    free(g_surnames);
  }
}

char *get_random_identity(PersonRole role) {
  const char *name = "Generico";
  const char *surname = "Rossi";
  int attempts = 0;

  do {
    // sesso (50/50)
    int is_female = rand() % 2;

    // nome
    if (is_female && g_count_f > 0) {
      name = g_names_f[rand() % g_count_f];
    } else if (!is_female && g_count_m > 0) {
      name = g_names_m[rand() % g_count_m];
    }

    // cognome
    if (g_count_s > 0) {
      surname = g_surnames[rand() % g_count_s];
    }

    size_t total_len = strlen(name) + 1 + strlen(surname);

    if (total_len < 20) {
      break;
    }

    attempts++;
  } while (attempts < 50);

  char *result = NULL;
  char text_buf[128];


  switch (role) {
  case ROLE_RESPONSABILE:
    snprintf(text_buf, sizeof(text_buf), "[RESPONSABILE] MARINA");
    asprintf(&result, COLOR_CYAN "%-28s" COLOR_RESET, text_buf);
    break;

  case ROLE_OPERATORE:
    snprintf(text_buf, sizeof(text_buf), "[OPERATORE] %s %c.", name,
             surname[0]);
    asprintf(&result, COLOR_GREEN "%-28s" COLOR_RESET, text_buf);
    break;

  case ROLE_CASSA:
    snprintf(text_buf, sizeof(text_buf), "[CASSA] %s %c.", name, surname[0]);
    asprintf(&result, COLOR_GREEN "%-28s" COLOR_RESET, text_buf);
    break;

  case ROLE_UTENTE:
    snprintf(text_buf, sizeof(text_buf), "[UTENTE] %s %s", name, surname);
    asprintf(&result, COLOR_LIGHT_GRAY "%-28s" COLOR_RESET, text_buf);
    break;

  default:
    snprintf(text_buf, sizeof(text_buf), "[?] %s", name);
    asprintf(&result, COLOR_YELLOW "%-28s" COLOR_RESET, text_buf);
    break;
  }

  return result;
}