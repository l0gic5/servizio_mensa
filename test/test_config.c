#include "common/config.h"
#include "unity.h"
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define DOUBLE_EPS 0.000001

#define TOTAL_CONFIG_FIELDS 71

#define TEST_CONFIG_FILE "test_simulation.conf"

static Config default_config_snapshot(void) {
  Config cfg;
  parse_config("non_existent.conf", &cfg);
  return cfg;
}

static bool double_diff(double a, double b) { return fabs(a - b) > DOUBLE_EPS; }

static int count_keys_in_conf_file(const char *filepath) {
  FILE *f = fopen(filepath, "r");
  if (!f) {
    return -1;
  }

  int count = 0;
  char line[256];

  while (fgets(line, sizeof(line), f)) {
    char *p = line;
    while (*p && isspace((unsigned char)*p)) {
      p++;
    }

    if (*p == '\0' || *p == '#') {
      continue;
    }

    if (strchr(p, '=')) {
      count++;
    }
  }

  fclose(f);
  return count;
}

static int count_config_differences(const Config *a, const Config *b) {
  int diff = 0;

#define CMP_INT(field)                                                         \
  if (a->field != b->field)                                                    \
  diff++
#define CMP_BOOL(field)                                                        \
  if (a->field != b->field)                                                    \
  diff++
#define CMP_DOUBLE(field)                                                      \
  if (double_diff(a->field, b->field))                                         \
  diff++
#define CMP_STR(field)                                                         \
  if (strcmp(a->field, b->field) != 0)                                         \
  diff++

  // 1) SIMULAZIONE GLOBALE
  CMP_INT(simulation_duration_days);
  CMP_INT(n_nanosecs_as_minute);
  CMP_INT(daily_service_minutes);
  CMP_INT(system_startup_delay_sec);
  CMP_INT(overload_threshold);

  // 2) POPOLAZIONE
  CMP_INT(nof_users);
  CMP_INT(nof_workers);
  CMP_INT(nof_table_seats);
  CMP_INT(avg_user_w_ticket);

  // 3) STAZIONI
  CMP_INT(queue_capacity_primi);
  CMP_INT(queue_capacity_secondi);
  CMP_INT(queue_capacity_caffe);
  CMP_INT(queue_capacity_cassa);

  CMP_INT(ticket_reader_capacity);
  CMP_INT(ticket_reader_timeout_ns);

  CMP_INT(workstations_primi);
  CMP_INT(workstations_secondi);
  CMP_INT(workstations_caffe);
  CMP_INT(workstations_cassa);

  // 4) METRICHE
  CMP_INT(avg_service_primi);
  CMP_INT(avg_service_secondi);
  CMP_INT(avg_service_caffe);
  CMP_INT(avg_service_cassa);

  CMP_INT(variability_primi);
  CMP_INT(variability_secondi);
  CMP_INT(variability_caffe);
  CMP_INT(variability_cassa);

  CMP_DOUBLE(price_primi);
  CMP_DOUBLE(price_secondi);
  CMP_DOUBLE(price_dolci);
  CMP_DOUBLE(price_caffe);
  CMP_DOUBLE(ticket_discount_percent);

  // 5) OPERATORI
  CMP_INT(max_pauses_per_day);
  CMP_INT(pause_duration_ns);
  CMP_INT(pause_probability_percent);
  CMP_INT(day_end_barrier_wait_sec);

  // 6) UTENTI
  CMP_INT(user_queue_timeout_sec);
  CMP_INT(user_meal_duration_ns);
  CMP_INT(user_coffee_duration_ns);
  CMP_INT(user_max_arrival_delay_us);

  CMP_INT(probability_user_wants_primo);
  CMP_INT(probability_user_wants_secondo);
  CMP_INT(probability_user_wants_dolce);
  CMP_INT(probability_user_wants_caffe);

  CMP_INT(user_budget_min);
  CMP_INT(user_budget_max);
  CMP_INT(user_min_daily_salary);
  CMP_INT(user_max_daily_salary);

  CMP_INT(max_groups);
  CMP_INT(max_users_per_group);

  // 7) LOGISTICA
  CMP_INT(daily_primi_count);
  CMP_INT(daily_secondi_count);
  CMP_INT(daily_dolci_count);
  CMP_INT(daily_caffe_count);

  CMP_INT(refill_interval_minutes);
  CMP_INT(refill_variance_percent);

  CMP_INT(avg_refill_time_ns);
  CMP_INT(avg_refill_primi);
  CMP_INT(avg_refill_secondi);
  CMP_INT(avg_refill_dolci);
  CMP_INT(avg_refill_caffe);

  CMP_INT(max_porzioni_primi);
  CMP_INT(max_porzioni_secondi);
  CMP_INT(max_porzioni_caffe);

  // 8) TOOLS
  CMP_INT(default_sciopero_stop_duration);

  CMP_STR(export_folder_path);
  CMP_BOOL(export_daily_reports_csv);
  CMP_BOOL(create_daily_single_files);
  CMP_STR(daily_reports_filename_csv);

  CMP_BOOL(export_final_stats_csv);
  CMP_STR(final_stats_filename_csv);

#undef CMP_INT
#undef CMP_BOOL
#undef CMP_DOUBLE
#undef CMP_STR

  return diff;
}

void setUp(void) {}

void tearDown(void) { remove(TEST_CONFIG_FILE); }

void test_file_not_found(void) {
  Config config;
  int res = parse_config("non_existent_file.conf", &config);
  TEST_ASSERT_EQUAL_INT(0, res);
  TEST_ASSERT_EQUAL_INT(30, config.simulation_duration_days);
}

void test_default_values_on_empty_file(void) {
  FILE *f = fopen(TEST_CONFIG_FILE, "w");
  fclose(f);

  Config config;
  int res = parse_config(TEST_CONFIG_FILE, &config);

  TEST_ASSERT_EQUAL_INT(0, res);
  TEST_ASSERT_EQUAL_INT(30, config.simulation_duration_days);
  TEST_ASSERT_EQUAL_INT(100000000, config.n_nanosecs_as_minute);
  TEST_ASSERT_EQUAL_INT(120, config.daily_service_minutes);
  TEST_ASSERT_EQUAL_INT(50, config.nof_users);
  TEST_ASSERT_EQUAL_INT(6, config.nof_workers);
  TEST_ASSERT_EQUAL_STRING("reports/", config.export_folder_path);
  TEST_ASSERT_EQUAL_FLOAT(25.0, config.ticket_discount_percent);
}

void test_coherence_with_real_default_file(void) {
  Config config;
  int res = parse_config("conf/default.conf", &config);

  if (res != 0) {
    TEST_IGNORE_MESSAGE(
        "File conf/default.conf non trovato, salto test di coerenza.");
    return;
  }

  // 1) SIMULAZIONE GLOBALE
  TEST_ASSERT_EQUAL_INT_MESSAGE(30, config.simulation_duration_days,
                                "simulation_duration_days");
  TEST_ASSERT_EQUAL_INT_MESSAGE(100000000, config.n_nanosecs_as_minute,
                                "n_nanosecs_as_minute");
  TEST_ASSERT_EQUAL_INT_MESSAGE(120, config.daily_service_minutes,
                                "daily_service_minutes");
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, config.system_startup_delay_sec,
                                "system_startup_delay_sec");
  TEST_ASSERT_EQUAL_INT_MESSAGE(50, config.overload_threshold,
                                "overload_threshold");

  // 2) POPOLAZIONE E RISORSE
  TEST_ASSERT_EQUAL_INT_MESSAGE(50, config.nof_users, "nof_users");
  TEST_ASSERT_EQUAL_INT_MESSAGE(6, config.nof_workers, "nof_workers");
  TEST_ASSERT_EQUAL_INT_MESSAGE(40, config.nof_table_seats, "nof_table_seats");
  TEST_ASSERT_EQUAL_INT_MESSAGE(80, config.avg_user_w_ticket,
                                "avg_user_w_ticket");

  // 3) STAZIONI (Capacità e Postazioni)
  TEST_ASSERT_EQUAL_INT_MESSAGE(15, config.queue_capacity_primi,
                                "queue_capacity_primi");
  TEST_ASSERT_EQUAL_INT_MESSAGE(15, config.queue_capacity_secondi,
                                "queue_capacity_secondi");
  TEST_ASSERT_EQUAL_INT_MESSAGE(20, config.queue_capacity_caffe,
                                "queue_capacity_caffe");
  TEST_ASSERT_EQUAL_INT_MESSAGE(20, config.queue_capacity_cassa,
                                "queue_capacity_cassa");
  TEST_ASSERT_EQUAL_INT_MESSAGE(3, config.ticket_reader_capacity,
                                "ticket_reader_capacity");
  TEST_ASSERT_EQUAL_INT_MESSAGE(5000000, config.ticket_reader_timeout_ns,
                                "ticket_reader_timeout_ns");
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, config.workstations_primi,
                                "workstations_primi");
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, config.workstations_secondi,
                                "workstations_secondi");
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, config.workstations_caffe,
                                "workstations_caffe");
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, config.workstations_cassa,
                                "workstations_cassa");

  // 4) METRICHE DI SERVIZIO
  TEST_ASSERT_EQUAL_INT_MESSAGE(100000000, config.avg_service_primi,
                                "avg_service_primi");
  TEST_ASSERT_EQUAL_INT_MESSAGE(120000000, config.avg_service_secondi,
                                "avg_service_secondi");
  TEST_ASSERT_EQUAL_INT_MESSAGE(50000000, config.avg_service_caffe,
                                "avg_service_caffe");
  TEST_ASSERT_EQUAL_INT_MESSAGE(60000000, config.avg_service_cassa,
                                "avg_service_cassa");
  TEST_ASSERT_EQUAL_INT_MESSAGE(30, config.variability_primi,
                                "variability_primi");
  TEST_ASSERT_EQUAL_INT_MESSAGE(30, config.variability_secondi,
                                "variability_secondi");
  TEST_ASSERT_EQUAL_INT_MESSAGE(20, config.variability_caffe,
                                "variability_caffe");
  TEST_ASSERT_EQUAL_INT_MESSAGE(20, config.variability_cassa,
                                "variability_cassa");
  TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(5.5, config.price_primi, "price_primi");
  TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(8.2, config.price_secondi, "price_secondi");
  TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(3.5, config.price_dolci, "price_dolci");
  TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(1.2, config.price_caffe, "price_caffe");
  TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(25.0, config.ticket_discount_percent,
                                   "ticket_discount_percent");

  // 5) OPERATORI
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, config.max_pauses_per_day,
                                "max_pauses_per_day");
  TEST_ASSERT_EQUAL_INT_MESSAGE(300000000, config.pause_duration_ns,
                                "pause_duration_ns");
  TEST_ASSERT_EQUAL_INT_MESSAGE(10, config.pause_probability_percent,
                                "pause_probability_percent");
  TEST_ASSERT_EQUAL_INT_MESSAGE(30, config.day_end_barrier_wait_sec,
                                "day_end_barrier_wait_sec");

  // 6) UTENTI
  TEST_ASSERT_EQUAL_INT_MESSAGE(5, config.user_queue_timeout_sec,
                                "user_queue_timeout_sec");
  TEST_ASSERT_EQUAL_INT_MESSAGE(2000000000, (int)config.user_meal_duration_ns,
                                "user_meal_duration_ns");
  TEST_ASSERT_EQUAL_INT_MESSAGE(10000000, config.user_coffee_duration_ns,
                                "user_coffee_duration_ns");
  TEST_ASSERT_EQUAL_INT_MESSAGE(9600000, config.user_max_arrival_delay_us,
                                "user_max_arrival_delay_us");
  TEST_ASSERT_EQUAL_INT_MESSAGE(60, config.probability_user_wants_primo,
                                "probability_user_wants_primo");
  TEST_ASSERT_EQUAL_INT_MESSAGE(70, config.probability_user_wants_secondo,
                                "probability_user_wants_secondo");
  TEST_ASSERT_EQUAL_INT_MESSAGE(40, config.probability_user_wants_dolce,
                                "probability_user_wants_dolce");
  TEST_ASSERT_EQUAL_INT_MESSAGE(20, config.probability_user_wants_caffe,
                                "probability_user_wants_caffe");
  TEST_ASSERT_EQUAL_INT_MESSAGE(10, config.user_budget_min, "user_budget_min");
  TEST_ASSERT_EQUAL_INT_MESSAGE(50, config.user_budget_max, "user_budget_max");
  TEST_ASSERT_EQUAL_INT_MESSAGE(5, config.user_min_daily_salary,
                                "user_min_daily_salary");
  TEST_ASSERT_EQUAL_INT_MESSAGE(15, config.user_max_daily_salary,
                                "user_max_daily_salary");

  TEST_ASSERT_EQUAL_INT_MESSAGE(30, config.max_groups, "max_groups");
  TEST_ASSERT_EQUAL_INT_MESSAGE(4, config.max_users_per_group,
                                "max_users_per_group");

  // 7) LOGISTICA
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, config.daily_primi_count,
                                "daily_primi_count");
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, config.daily_secondi_count,
                                "daily_secondi_count");
  TEST_ASSERT_EQUAL_INT_MESSAGE(4, config.daily_dolci_count,
                                "daily_dolci_count");
  TEST_ASSERT_EQUAL_INT_MESSAGE(5, config.daily_caffe_count,
                                "daily_caffe_count");

  TEST_ASSERT_EQUAL_INT_MESSAGE(10, config.refill_interval_minutes,
                                "refill_interval_minutes");
  TEST_ASSERT_EQUAL_INT_MESSAGE(20, config.refill_variance_percent,
                                "refill_variance_percent");

  TEST_ASSERT_EQUAL_INT_MESSAGE(50000000, config.avg_refill_time_ns,
                                "avg_refill_time_ns");
  TEST_ASSERT_EQUAL_INT_MESSAGE(20, config.avg_refill_primi,
                                "avg_refill_primi");
  TEST_ASSERT_EQUAL_INT_MESSAGE(20, config.avg_refill_secondi,
                                "avg_refill_secondi");
  TEST_ASSERT_EQUAL_INT_MESSAGE(30, config.avg_refill_dolci,
                                "avg_refill_dolci");
  TEST_ASSERT_EQUAL_INT_MESSAGE(500, config.avg_refill_caffe,
                                "avg_refill_caffe");

  TEST_ASSERT_EQUAL_INT_MESSAGE(50, config.max_porzioni_primi,
                                "max_porzioni_primi");
  TEST_ASSERT_EQUAL_INT_MESSAGE(50, config.max_porzioni_secondi,
                                "max_porzioni_secondi");
  TEST_ASSERT_EQUAL_INT_MESSAGE(75, config.max_porzioni_dolci,
                                "max_porzioni_dolci");
  TEST_ASSERT_EQUAL_INT_MESSAGE(1000, config.max_porzioni_caffe,
                                "max_porzioni_caffe");

  // 8) TOOLS
  TEST_ASSERT_EQUAL_INT_MESSAGE(60, config.default_sciopero_stop_duration,
                                "default_sciopero_stop_duration");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("reports/", config.export_folder_path,
                                   "export_folder_path");
  TEST_ASSERT_TRUE_MESSAGE(config.export_daily_reports_csv,
                           "export_daily_reports_csv");
  TEST_ASSERT_FALSE_MESSAGE(config.create_daily_single_files,
                            "create_daily_single_files");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("days/daily_report",
                                   config.daily_reports_filename_csv,
                                   "daily_reports_filename_csv");
  TEST_ASSERT_TRUE_MESSAGE(config.export_final_stats_csv,
                           "export_final_stats_csv");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("final_stats",
                                   config.final_stats_filename_csv,
                                   "final_stats_filename_csv");
}

void test_all_conf_files_override_at_least_one_field(void) {
  DIR *dir = opendir("conf");
  TEST_ASSERT_NOT_NULL(dir);

  Config base = default_config_snapshot();
  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    if (!strstr(entry->d_name, ".conf")) {
      continue;
    }

    if (strcmp(entry->d_name, "default.conf") == 0) {
      continue;
    }

    char path[512];
    snprintf(path, sizeof(path), "conf/%s", entry->d_name);

    Config cfg;
    parse_config(path, &cfg);

    int changed = count_config_differences(&base, &cfg);

    char msg[512];
    snprintf(msg, sizeof(msg), "Il file %s non modifica alcun campo di Config",
             entry->d_name);

    TEST_ASSERT_MESSAGE(changed > 0, msg);
  }

  closedir(dir);
}

void test_full_configuration_override(void) {
  const int TOTAL_FIELDS = TOTAL_CONFIG_FIELDS;

  FILE *f = fopen(TEST_CONFIG_FILE, "w");

  // SIMULAZIONE GLOBALE (5)
  fprintf(f, "SIM_DURATION=999\n");
  fprintf(f, "N_NANO_SECS=999\n");
  fprintf(f, "DAILY_SERVICE_MINUTES=999\n");

  // CORRETTO: Aggiunto _SEC
  fprintf(f, "SYSTEM_STARTUP_DELAY_SEC=999\n");

  fprintf(f, "OVERLOAD_THRESHOLD=999\n");

  // POPOLAZIONE (4)
  fprintf(f, "NOF_USERS=999\n");
  fprintf(f, "NOF_WORKERS=999\n");
  fprintf(f, "NOF_TABLE_SEATS=999\n");
  fprintf(f, "AVG_USER_W_TICKET=99\n");

  // STAZIONI (10)
  fprintf(f, "NOF_WK_SEATS_PRIMI=999\n");
  fprintf(f, "NOF_WK_SEATS_SECONDI=999\n");
  fprintf(f, "NOF_WK_SEATS_CAFFE=999\n");
  fprintf(f, "NOF_WK_SEATS_CASSA=999\n");

  // Ticket Reader
  fprintf(f, "TICKET_READER_CAPACITY=999\n");
  fprintf(f, "TICKET_READER_TIMEOUT_NS=999\n");
  // Postazioni attive
  fprintf(f, "WORKSTATIONS_PRIMI=999\n");
  fprintf(f, "WORKSTATIONS_SECONDI=999\n");
  fprintf(f, "WORKSTATIONS_CAFFE=999\n");
  fprintf(f, "WORKSTATIONS_CASSA=999\n");

  // METRICHE (12)
  // Tempi servizio
  fprintf(f, "AVG_SRVC_PRIMI=999\n");
  fprintf(f, "AVG_SRVC_SECONDI=999\n");
  fprintf(f, "AVG_SRVC_CAFFE=999\n");
  fprintf(f, "AVG_SRVC_CASSA=999\n");

  fprintf(f, "VARIABILITY_PRIMI=99\n");
  fprintf(f, "VARIABILITY_SECONDI=99\n");
  fprintf(f, "VARIABILITY_CAFFE=99\n");
  fprintf(f, "VARIABILITY_CASSA=99\n");

  // Prezzi
  fprintf(f, "PRICE_PRIMI=99.9\n");
  fprintf(f, "PRICE_SECONDI=99.9\n");
  fprintf(f, "PRICE_DOLCI=99.9\n");
  fprintf(f, "PRICE_CAFFE=99.9\n");
  fprintf(f, "TICKET_DISCOUNT_PERCENT=99.9\n");

  // OPERATORI (4)
  fprintf(f, "MAX_PAUSES_PER_DAY=99\n");
  fprintf(f, "PAUSE_DURATION_NS=999\n");
  fprintf(f, "PAUSE_PROBABILITY_PERCENT=99\n");
  fprintf(f, "DAY_END_BARRIER_WAIT_SEC=999\n");

  // UTENTI (11)
  // Tempi
  fprintf(f, "USER_QUEUE_TIMEOUT_SEC=999\n");
  fprintf(f, "USER_MEAL_DURATION_NS=999\n");
  fprintf(f, "USER_COFFEE_DURATION_NS=999\n");
  fprintf(f, "USER_MAX_ARRIVAL_DELAY_US=999\n");

  fprintf(f, "PROBABILITY_USER_WANTS_PRIMO=99\n");
  fprintf(f, "PROBABILITY_USER_WANTS_SECONDO=99\n");
  fprintf(f, "PROBABILITY_USER_WANTS_DOLCE=99\n");
  fprintf(f, "PROBABILITY_USER_WANTS_CAFFE=99\n");

  // Budget
  fprintf(f, "USER_BUDGET_MIN=999\n");
  fprintf(f, "USER_BUDGET_MAX=9999\n");
  fprintf(f, "USER_MIN_DAILY_SALARY=999\n");
  fprintf(f, "USER_MAX_DAILY_SALARY=9999\n");

  fprintf(f, "MAX_GROUPS=9999\n");
  fprintf(f, "MAX_USERS_PER_GROUP=9999\n");

  // LOGISTICA (13)
  fprintf(f, "DAILY_PRIMI_COUNT=9\n");
  fprintf(f, "DAILY_SECONDI_COUNT=9\n");
  fprintf(f, "DAILY_DOLCI_COUNT=9\n");
  fprintf(f, "DAILY_CAFFE_COUNT=9\n");
  fprintf(f, "REFILL_INTERVAL_MINUTES=999\n");
  fprintf(f, "REFILL_VARIANCE_PERCENT=99\n");
  fprintf(f, "AVG_REFILL_TIME_NS=999\n");
  fprintf(f, "AVG_REFILL_PRIMI=999\n");
  fprintf(f, "AVG_REFILL_SECONDI=999\n");
  fprintf(f, "AVG_REFILL_DOLCI=999\n");
  fprintf(f, "AVG_REFILL_CAFFE=999\n");
  fprintf(f, "MAX_PORZIONI_PRIMI=999\n");
  fprintf(f, "MAX_PORZIONI_SECONDI=999\n");
  fprintf(f, "MAX_PORZIONI_DOLCI=999\n");
  fprintf(f, "MAX_PORZIONI_CAFFE=999\n");

  // TOOLS & SCIOPERO (7)
  fprintf(f, "DEFAULT_SCIOPERO_STOP_DURATION=999\n");

  fprintf(f, "EXPORT_FOLDER_PATH=override_folder/\n");

  fprintf(f, "EXPORT_DAILY_REPORTS_CSV=0\n");
  fprintf(f, "CREATE_DAILY_SINGLE_FILES=1\n");
  fprintf(f, "DAILY_REPORTS_FILENAME_CSV=override_daily\n");

  fprintf(f, "EXPORT_FINAL_STATS_CSV=0\n");
  fprintf(f, "FINAL_STATS_FILENAME_CSV=override_stats\n");

  fclose(f);

  // parsing
  Config base = default_config_snapshot();
  Config full_override;
  int res = parse_config(TEST_CONFIG_FILE, &full_override);

  TEST_ASSERT_EQUAL_INT_MESSAGE(0, res,
                                "Il parsing del file completo è fallito");

  int diffs = count_config_differences(&base, &full_override);

  char error_msg[100];
  snprintf(error_msg, sizeof(error_msg),
           "Mi aspettavo %d campi modificati, ma ne ho trovati %d",
           TOTAL_FIELDS, diffs);

  TEST_ASSERT_EQUAL_INT_MESSAGE(TOTAL_FIELDS, diffs, error_msg);
}

void test_consistency_with_default_conf_count(void) {
  int keys_in_file = count_keys_in_conf_file("conf/default.conf");
  
  if (keys_in_file == -1) {
    TEST_IGNORE_MESSAGE("File conf/default.conf non trovato, salto il controllo conteggio.");
    return;
  }

  char msg[128];
  snprintf(msg, sizeof(msg), 
           "Il numero di chiavi in default.conf (%d) non coincide con TOTAL_CONFIG_FIELDS (%d)", 
           keys_in_file, TOTAL_CONFIG_FIELDS);
           
  TEST_ASSERT_EQUAL_INT_MESSAGE(TOTAL_CONFIG_FIELDS, keys_in_file, msg);
}

void test_parse_valid_file(void) {
  FILE *f = fopen(TEST_CONFIG_FILE, "w");
  fprintf(f, "SIM_DURATION=45\n");
  fprintf(f, "NOF_USERS=100\n");
  fprintf(f, "PRICE_PRIMI=7.5\n");
  fprintf(f, "EXPORT_DAILY_REPORTS_CSV=true\n");
  fprintf(f, "EXPORT_FOLDER_PATH=/tmp/output\n");
  fclose(f);

  Config config;
  int res = parse_config(TEST_CONFIG_FILE, &config);

  TEST_ASSERT_EQUAL_INT(0, res);
  TEST_ASSERT_EQUAL_INT(45, config.simulation_duration_days);
  TEST_ASSERT_EQUAL_INT(100, config.nof_users);
  TEST_ASSERT_EQUAL_DOUBLE(7.5, config.price_primi);
  TEST_ASSERT_EQUAL_INT(1, config.export_daily_reports_csv);
  TEST_ASSERT_EQUAL_STRING("/tmp/output", config.export_folder_path);
}

void test_partial_overrides(void) {
  FILE *f = fopen(TEST_CONFIG_FILE, "w");
  fprintf(f, "N_NANO_SECS=5000\n");
  fclose(f);

  Config config;
  parse_config(TEST_CONFIG_FILE, &config);

  TEST_ASSERT_EQUAL_INT(5000, config.n_nanosecs_as_minute);
  TEST_ASSERT_EQUAL_INT(30, config.simulation_duration_days);
}

void test_comments_and_empty_lines(void) {
  FILE *f = fopen(TEST_CONFIG_FILE, "w");
  fprintf(f, "# Commento iniziale\n\n");
  fprintf(f, "SIM_DURATION=20\n");
  fprintf(f, "   # Commento indentato\n");
  fprintf(f, "\n");
  fprintf(f, "NOF_WORKERS=12\n");
  fclose(f);

  Config config;
  parse_config(TEST_CONFIG_FILE, &config);

  TEST_ASSERT_EQUAL_INT(20, config.simulation_duration_days);
  TEST_ASSERT_EQUAL_INT(12, config.nof_workers);
  // Verifica che le linee vuote non abbiano resettato i default
  TEST_ASSERT_EQUAL_INT(15, config.queue_capacity_primi);
}

void test_malformed_lines(void) {
  FILE *f = fopen(TEST_CONFIG_FILE, "w");
  fprintf(f, "SIM_DURATION=25\n");
  fprintf(f, "=50\n");
  fprintf(f, "NOF_USERS=\n");
  fprintf(f, "INVALID_KEY=10\n");
  fprintf(f, "=\n");
  fclose(f);

  Config config;
  parse_config(TEST_CONFIG_FILE, &config);

  TEST_ASSERT_EQUAL_INT(25, config.simulation_duration_days);
  TEST_ASSERT_EQUAL_INT(50, config.nof_users);
}

void test_non_numeric_values(void) {
  FILE *f = fopen(TEST_CONFIG_FILE, "w");
  fprintf(f, "SIM_DURATION=not_a_number\n");
  fprintf(f, "PRICE_PRIMI=abc\n");
  fclose(f);

  Config config;
  parse_config(TEST_CONFIG_FILE, &config);

  // atoi e atof restituiscono 0 in caso di stringa non numerica
  TEST_ASSERT_EQUAL_INT(0, config.simulation_duration_days);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, config.price_primi);
}

void test_unknown_keys_ignored(void) {
  FILE *f = fopen(TEST_CONFIG_FILE, "w");
  fprintf(f, "CHIAVE_INVENTATA=100\n");
  fprintf(f, "SIM_DURATION=10\n");
  fclose(f);

  Config config;
  parse_config(TEST_CONFIG_FILE, &config);

  TEST_ASSERT_EQUAL_INT(10, config.simulation_duration_days);
}

void test_boolean_parsing_variations(void) {
  FILE *f = fopen(TEST_CONFIG_FILE, "w");
  fprintf(f, "EXPORT_DAILY_REPORTS_CSV=1\n");
  fprintf(f, "CREATE_DAILY_SINGLE_FILES=true\n");
  fprintf(f, "EXPORT_FINAL_STATS_CSV=0\n");
  fclose(f);

  Config config;
  parse_config(TEST_CONFIG_FILE, &config);

  TEST_ASSERT_TRUE(config.export_daily_reports_csv);
  TEST_ASSERT_TRUE(config.create_daily_single_files);
  TEST_ASSERT_FALSE(config.export_final_stats_csv);
}

void test_extreme_line_length(void) {
  FILE *f = fopen(TEST_CONFIG_FILE, "w");
  fprintf(f, "SIM_DURATION=");
  for (int i = 0; i < 600; i++) {
    // più lunga di MAX_LINE_LENGTH
    fprintf(f, "9");
  }
  fprintf(f, "\nNOF_USERS=10\n");
  fclose(f);

  Config config;
  int res = parse_config(TEST_CONFIG_FILE, &config);
  TEST_ASSERT_EQUAL_INT(0, res);
  TEST_ASSERT_EQUAL_INT(10, config.nof_users);
}

void test_duplicate_keys_precedence(void) {
  FILE *f = fopen(TEST_CONFIG_FILE, "w");
  fprintf(f, "SIM_DURATION=10\n");
  fprintf(f, "SIM_DURATION=20\n");
  fprintf(f, "SIM_DURATION=30\n");
  fclose(f);

  Config config;
  parse_config(TEST_CONFIG_FILE, &config);

  TEST_ASSERT_EQUAL_INT(30, config.simulation_duration_days);
}

void test_parse_all_fields(void) {
  FILE *f = fopen(TEST_CONFIG_FILE, "w");
  // Simulazione Globale
  fprintf(f, "SIM_DURATION=10\n");
  fprintf(f, "N_NANO_SECS=1000\n");
  fprintf(f, "DAILY_SERVICE_MINUTES=60\n");
  fprintf(f, "OVERLOAD_THRESHOLD=100\n");
  // Popolazione
  fprintf(f, "NOF_USERS=200\n");
  fprintf(f, "NOF_WORKERS=40\n");
  // Stazioni
  fprintf(f, "NOF_WK_SEATS_PRIMI=30\n");
  fprintf(f, "WORKSTATIONS_PRIMI=5\n");
  // Metriche
  fprintf(f, "AVG_SRVC_PRIMI=5000\n");
  fprintf(f, "PRICE_PRIMI=5.50\n");
  fprintf(f, "TICKET_DISCOUNT_PERCENT=15.5\n");
  // Logistica
  fprintf(f, "MAX_PORZIONI_PRIMI=500\n");
  // Tools
  fprintf(f, "EXPORT_DAILY_REPORTS_CSV=1\n");
  fprintf(f, "CREATE_DAILY_SINGLE_FILES=true\n");
  fprintf(f, "DAILY_REPORTS_FILENAME_CSV=test_daily\n");
  fclose(f);

  Config config;
  parse_config(TEST_CONFIG_FILE, &config);

  TEST_ASSERT_EQUAL_INT(10, config.simulation_duration_days);
  TEST_ASSERT_EQUAL_INT(1000, config.n_nanosecs_as_minute);
  TEST_ASSERT_EQUAL_INT(60, config.daily_service_minutes);
  TEST_ASSERT_EQUAL_INT(100, config.overload_threshold);
  TEST_ASSERT_EQUAL_INT(200, config.nof_users);
  TEST_ASSERT_EQUAL_INT(40, config.nof_workers);
  TEST_ASSERT_EQUAL_INT(30, config.queue_capacity_primi);
  TEST_ASSERT_EQUAL_INT(5, config.workstations_primi);
  TEST_ASSERT_EQUAL_INT(5000, config.avg_service_primi);
  TEST_ASSERT_EQUAL_DOUBLE(5.50, config.price_primi);
  TEST_ASSERT_EQUAL_DOUBLE(15.5, config.ticket_discount_percent);
  TEST_ASSERT_EQUAL_INT(500, config.max_porzioni_primi);
  TEST_ASSERT_EQUAL_INT(1, config.export_daily_reports_csv);
  TEST_ASSERT_EQUAL_INT(1, config.create_daily_single_files);
  TEST_ASSERT_EQUAL_STRING("test_daily", config.daily_reports_filename_csv);
}

void test_trim_functionality(void) {
  FILE *f = fopen(TEST_CONFIG_FILE, "w");
  fprintf(f, "  SIM_DURATION  =  50  \n");
  fprintf(f, "NOF_USERS=  80\n");
  fclose(f);

  Config config;
  parse_config(TEST_CONFIG_FILE, &config);

  TEST_ASSERT_EQUAL_INT(50, config.simulation_duration_days);
  TEST_ASSERT_EQUAL_INT(80, config.nof_users);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_file_not_found);
  RUN_TEST(test_default_values_on_empty_file);
  RUN_TEST(test_coherence_with_real_default_file);
  RUN_TEST(test_all_conf_files_override_at_least_one_field);
  RUN_TEST(test_full_configuration_override);
  RUN_TEST(test_consistency_with_default_conf_count);

  RUN_TEST(test_parse_valid_file);
  RUN_TEST(test_partial_overrides);

  RUN_TEST(test_comments_and_empty_lines);
  RUN_TEST(test_malformed_lines);
  RUN_TEST(test_non_numeric_values);
  RUN_TEST(test_unknown_keys_ignored);

  RUN_TEST(test_boolean_parsing_variations);
  RUN_TEST(test_extreme_line_length);
  RUN_TEST(test_duplicate_keys_precedence);

  RUN_TEST(test_parse_all_fields);
  RUN_TEST(test_trim_functionality);
  return UNITY_END();
}