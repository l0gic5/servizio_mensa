#include "common/config.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

#define TEST_CONFIG_FILE "test_simulation.conf"

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
  TEST_ASSERT_EQUAL_INT(30, config.simulation_duration_days);
  TEST_ASSERT_EQUAL_INT(100000000, config.n_nanosecs_as_minute);
  TEST_ASSERT_EQUAL_INT(120, config.daily_service_minutes);
  TEST_ASSERT_EQUAL_INT(2, config.system_startup_delay_sec);
  TEST_ASSERT_EQUAL_INT(50, config.overload_threshold);

  // 2) POPOLAZIONE E RISORSE
  TEST_ASSERT_EQUAL_INT(50, config.nof_users);
  TEST_ASSERT_EQUAL_INT(6, config.nof_workers);
  TEST_ASSERT_EQUAL_INT(40, config.nof_table_seats);
  TEST_ASSERT_EQUAL_INT(80, config.avg_user_w_ticket);

  // 3) STAZIONI (Capacità e Postazioni)
  TEST_ASSERT_EQUAL_INT(15, config.queue_capacity_primi);
  TEST_ASSERT_EQUAL_INT(15, config.queue_capacity_secondi);
  TEST_ASSERT_EQUAL_INT(20, config.queue_capacity_caffe);
  TEST_ASSERT_EQUAL_INT(20, config.queue_capacity_cassa);
  TEST_ASSERT_EQUAL_INT(4, config.ticket_reader_capacity);
  TEST_ASSERT_EQUAL_INT(5000000, config.ticket_reader_timeout_ns);
  TEST_ASSERT_EQUAL_INT(2, config.workstations_primi);
  TEST_ASSERT_EQUAL_INT(2, config.workstations_secondi);
  TEST_ASSERT_EQUAL_INT(2, config.workstations_caffe);
  TEST_ASSERT_EQUAL_INT(1, config.workstations_cassa);

  // 4) METRICHE DI SERVIZIO
  TEST_ASSERT_EQUAL_INT(100000000, config.avg_service_primi);
  TEST_ASSERT_EQUAL_INT(120000000, config.avg_service_secondi);
  TEST_ASSERT_EQUAL_INT(50000000, config.avg_service_caffe);
  TEST_ASSERT_EQUAL_INT(60000000, config.avg_service_cassa);
  TEST_ASSERT_EQUAL_INT(30, config.variability_primi);
  TEST_ASSERT_EQUAL_INT(30, config.variability_secondi);
  TEST_ASSERT_EQUAL_INT(20, config.variability_caffe);
  TEST_ASSERT_EQUAL_INT(20, config.variability_cassa);
  TEST_ASSERT_EQUAL_DOUBLE(5.5, config.price_primi);
  TEST_ASSERT_EQUAL_DOUBLE(8.2, config.price_secondi);
  TEST_ASSERT_EQUAL_DOUBLE(1.2, config.price_caffe);
  TEST_ASSERT_EQUAL_DOUBLE(25.0, config.ticket_discount_percent);

  // 5) OPERATORI
  TEST_ASSERT_EQUAL_INT(2, config.max_pauses_per_day);
  TEST_ASSERT_EQUAL_INT(300000000, config.pause_duration_ns);
  TEST_ASSERT_EQUAL_INT(10, config.pause_probability_percent);
  TEST_ASSERT_EQUAL_INT(30, config.day_end_barrier_wait_sec);

  // 6) UTENTI
  TEST_ASSERT_EQUAL_INT(5, config.user_queue_timeout_sec);
  TEST_ASSERT_EQUAL_INT(2000000000, (int)config.user_meal_duration_ns);
  TEST_ASSERT_EQUAL_INT(10000000, config.user_coffee_duration_ns);
  TEST_ASSERT_EQUAL_INT(9600000, config.user_max_arrival_delay_us);
  TEST_ASSERT_EQUAL_INT(60, config.probability_user_wants_primo);
  TEST_ASSERT_EQUAL_INT(70, config.probability_user_wants_secondo);
  TEST_ASSERT_EQUAL_INT(20, config.probability_user_wants_caffe);
  TEST_ASSERT_EQUAL_INT(10, config.user_budget_min);
  TEST_ASSERT_EQUAL_INT(50, config.user_budget_max);
  TEST_ASSERT_EQUAL_INT(5, config.user_min_daily_salary);
  TEST_ASSERT_EQUAL_INT(15, config.user_max_daily_salary);

  // 7) LOGISTICA
  TEST_ASSERT_EQUAL_INT(10, config.refill_interval_minutes);
  TEST_ASSERT_EQUAL_INT(20, config.avg_refill_primi);
  TEST_ASSERT_EQUAL_INT(20, config.avg_refill_secondi);
  TEST_ASSERT_EQUAL_INT(50, config.max_porzioni_primi);
  TEST_ASSERT_EQUAL_INT(50, config.max_porzioni_secondi);
  TEST_ASSERT_EQUAL_INT(1000, config.max_porzioni_caffe);

  // 8) TOOLS
  TEST_ASSERT_EQUAL_INT(60, config.default_sciopero_stop_duration);
  TEST_ASSERT_EQUAL_STRING("reports/", config.export_folder_path);
  TEST_ASSERT_TRUE(config.export_daily_reports_csv);
  TEST_ASSERT_FALSE(config.create_daily_single_files);
  TEST_ASSERT_EQUAL_STRING("days/daily_report",
                           config.daily_reports_filename_csv);
  TEST_ASSERT_TRUE(config.export_final_stats_csv);
  TEST_ASSERT_EQUAL_STRING("final_stats", config.final_stats_filename_csv);
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