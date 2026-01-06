#include "common/config.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEMP_CONFIG_FILE "test_config.tmp"
#define ACTUAL_DEFAULT_CONF_PATH "conf/default.conf"

static Config test_config;

////////////////////////////
//  FUNZIONI DI SUPPORTO  //
////////////////////////////

static void create_temp_config_file(const char *content) {
  FILE *fp = fopen(TEMP_CONFIG_FILE, "w");
  if (fp) {
    fputs(content, fp);
    fclose(fp);
  }
}

void setUp(void) { memset(&test_config, 0, sizeof(Config)); }

void tearDown(void) { unlink(TEMP_CONFIG_FILE); }

///////////////
//  PARSING  //
///////////////

static void test_file_not_found(void) {
  int result = parse_config("non_existent_file.conf", &test_config);
  TEST_ASSERT_EQUAL_INT(-1, result);
}

static void test_parse_valid_file(void) {
  // SIM_DURATION => non è di default
  const char *file_content = "# Commento ignorato\n"
                             "SIM_DURATION = 99\n"
                             "NOF_USERS=50\n"
                             "PRICE_PRIMI =  5 \n";

  create_temp_config_file(file_content);

  int result = parse_config(TEMP_CONFIG_FILE, &test_config);

  TEST_ASSERT_EQUAL_INT(0, result);
  TEST_ASSERT_EQUAL_INT(99, test_config.simulation_duration_days);
  TEST_ASSERT_EQUAL_INT(50, test_config.nof_users);
  TEST_ASSERT_EQUAL_INT(5, test_config.price_primi);

  TEST_ASSERT_EQUAL_INT(8, test_config.price_secondi);
}

/////////////////////////////////
//  DEFAULT VALUES & OVERRIDES //
/////////////////////////////////

static void test_default_values_on_empty_file(void) {
  // create an empty file
  create_temp_config_file("");

  int result = parse_config(TEMP_CONFIG_FILE, &test_config);

  TEST_ASSERT_EQUAL_INT(0, result);

  // simulation & users defaults
  TEST_ASSERT_EQUAL_INT(30, test_config.simulation_duration_days);
  TEST_ASSERT_EQUAL_INT(100000000, test_config.n_nanosecs_as_minute);
  TEST_ASSERT_EQUAL_INT(120, test_config.daily_service_minutes);
  TEST_ASSERT_EQUAL_INT(40, test_config.nof_users);
  TEST_ASSERT_EQUAL_INT(6, test_config.nof_workers);
  TEST_ASSERT_EQUAL_INT(30, test_config.nof_table_seats);
  TEST_ASSERT_EQUAL_INT(10, test_config.overload_threshold);

  // service times defaults
  TEST_ASSERT_EQUAL_INT(5000, test_config.avg_service_primi);
  TEST_ASSERT_EQUAL_INT(6000, test_config.avg_service_secondi);
  TEST_ASSERT_EQUAL_INT(2000, test_config.avg_service_coffee);
  TEST_ASSERT_EQUAL_INT(3000, test_config.avg_service_cassa);

  // workstations defaults
  TEST_ASSERT_EQUAL_INT(5, test_config.workstations_primi);
  TEST_ASSERT_EQUAL_INT(5, test_config.workstations_secondi);
  TEST_ASSERT_EQUAL_INT(5, test_config.workstations_coffee);
  TEST_ASSERT_EQUAL_INT(1, test_config.workstations_cassa);

  // pauses defaults
  TEST_ASSERT_EQUAL_INT(3, test_config.max_pauses_per_day);
  TEST_ASSERT_EQUAL_INT(500000000, test_config.pause_duration_ns);
  TEST_ASSERT_EQUAL_INT(10, test_config.pause_probability_percent);

  // queue seats defaults
  TEST_ASSERT_EQUAL_INT(10, test_config.queue_capacity_primi);
  TEST_ASSERT_EQUAL_INT(10, test_config.queue_capacity_secondi);
  TEST_ASSERT_EQUAL_INT(15, test_config.queue_capacity_coffee);
  TEST_ASSERT_EQUAL_INT(15, test_config.queue_capacity_cassa);

  // prices defaults
  TEST_ASSERT_EQUAL_INT(5, test_config.price_primi);
  TEST_ASSERT_EQUAL_INT(8, test_config.price_secondi);
  TEST_ASSERT_EQUAL_INT(1, test_config.price_coffee);

  // refills defaults
  TEST_ASSERT_EQUAL_INT(50000, test_config.avg_refill_primi);
  TEST_ASSERT_EQUAL_INT(50000, test_config.avg_refill_secondi);
  TEST_ASSERT_EQUAL_INT(100, test_config.max_porzioni_primi);
  TEST_ASSERT_EQUAL_INT(100, test_config.max_porzioni_secondi);

  // variability defaults
  TEST_ASSERT_EQUAL_INT(50, test_config.variability_primi);
  TEST_ASSERT_EQUAL_INT(50, test_config.variability_secondi);
  TEST_ASSERT_EQUAL_INT(80, test_config.variability_coffee);
  TEST_ASSERT_EQUAL_INT(10, test_config.variability_cassa);
}

static void test_partial_overrides(void) {
  // only override simulation duration and one price
  const char *file_content = "SIM_DURATION = 12345\n"
                             "PRICE_COFFEE = 99\n";

  create_temp_config_file(file_content);

  parse_config(TEMP_CONFIG_FILE, &test_config);

  // check overrides
  TEST_ASSERT_EQUAL_INT(12345, test_config.simulation_duration_days);
  TEST_ASSERT_EQUAL_INT(99, test_config.price_coffee);

  // check that other values are still defaults
  // default
  TEST_ASSERT_EQUAL_INT(6, test_config.nof_workers);
  // default
  TEST_ASSERT_EQUAL_INT(5000, test_config.avg_service_primi);
}

static void test_actual_default_conf_file(void) {
  FILE *fp = fopen(ACTUAL_DEFAULT_CONF_PATH, "r");
  if (!fp) {
    TEST_IGNORE_MESSAGE(
        "Skipping: conf/default.conf not found from current PWD.");
    return;
  }
  fclose(fp);

  int result = parse_config(ACTUAL_DEFAULT_CONF_PATH, &test_config);
  TEST_ASSERT_EQUAL_INT(0, result);

  TEST_ASSERT_EQUAL_INT(30, test_config.simulation_duration_days);
  TEST_ASSERT_EQUAL_INT(40, test_config.nof_users);
  TEST_ASSERT_EQUAL_INT(6, test_config.nof_workers);
  TEST_ASSERT_EQUAL_INT(5000, test_config.avg_service_primi);
}

/////////////////
//  EDGE CASE  //
/////////////////

static void test_comments_and_empty_lines(void) {
  const char *file_content = "\n"
                             "# Solo commenti\n"
                             "\n";

  create_temp_config_file(file_content);

  int result = parse_config(TEMP_CONFIG_FILE, &test_config);

  TEST_ASSERT_EQUAL_INT(0, result);
  // should default to 30
  TEST_ASSERT_EQUAL_INT(30, test_config.simulation_duration_days);
}

//////////////////
//  ROBUSTNESS  //
//////////////////

static void test_malformed_lines(void) {
  const char *file_content = "SIM_DURATION\n"
                             " = 50\n"
                             "NOF_USERS =\n"
                             "PRICE_PRIMI = 10 = 20\n"
                             "   =   \n";

  create_temp_config_file(file_content);

  int result = parse_config(TEMP_CONFIG_FILE, &test_config);

  TEST_ASSERT_EQUAL_INT(0, result);

  TEST_ASSERT_EQUAL_INT(30, test_config.simulation_duration_days);
  TEST_ASSERT_EQUAL_INT(40, test_config.nof_users);
}

static void test_unknown_keys_ignored(void) {
  const char *file_content = "CHIAVE_INVENTATA = 100\n"
                             "SIM_DURATION = 5\n"
                             "ALTRA_CHIAVE = 999\n";

  create_temp_config_file(file_content);

  int result = parse_config(TEMP_CONFIG_FILE, &test_config);

  TEST_ASSERT_EQUAL_INT(0, result);
  TEST_ASSERT_EQUAL_INT(5, test_config.simulation_duration_days);
}

static void test_non_numeric_values(void) {
  const char *file_content = "SIM_DURATION = trenta\n"
                             "NOF_USERS = 10abc\n";

  create_temp_config_file(file_content);

  parse_config(TEMP_CONFIG_FILE, &test_config);

  // atoi("trenta") == 0 -> triggers default -> 30
  TEST_ASSERT_EQUAL_INT(30, test_config.simulation_duration_days);
  // atoi("10abc") == 10 -> Valid override
  TEST_ASSERT_EQUAL_INT(10, test_config.nof_users);
}

static void test_random_parameters_parsing(void) {
  const char *file_content = "USER_QUEUE_TIMEOUT_SEC = 5\n"
                             "USER_MEAL_DURATION_NS = 123456\n"
                             "USER_MAX_ARRIVAL_DELAY_US = 999\n"

                             "PROBABILITY_USER_WANTS_PRIMO = 80\n"
                             "PROBABILITY_USER_WANTS_SECONDO = 15\n"
                             "PROBABILITY_USER_WANTS_COFFEE = 5\n";

  create_temp_config_file(file_content);

  int result = parse_config(TEMP_CONFIG_FILE, &test_config);

  TEST_ASSERT_EQUAL_INT(0, result);

  TEST_ASSERT_EQUAL_INT(5, test_config.user_queue_timeout_sec);
  TEST_ASSERT_EQUAL_INT(123456, test_config.user_meal_duration_ns);
  TEST_ASSERT_EQUAL_INT(999, test_config.user_max_arrival_delay_us);

  TEST_ASSERT_EQUAL_INT(80, test_config.probability_user_wants_primo);
  TEST_ASSERT_EQUAL_INT(15, test_config.probability_user_wants_secondo);
  TEST_ASSERT_EQUAL_INT(5, test_config.probability_user_wants_coffee);
}

///////////////////
//  INTEGRATION  //
///////////////////

static void test_parse_all_fields(void) {
  const char *file_content = "SIM_DURATION = 1\n"
                             "NOF_USERS = 2\n"
                             "NOF_WORKERS = 3\n"
                             "NOF_TABLE_SEATS = 4\n"
                             "OVERLOAD_THRESHOLD = 999\n"

                             "AVG_SRVC_PRIMI = 10\n"
                             "AVG_SRVC_SECONDI = 11\n"
                             "AVG_SRVC_COFFEE = 12\n"
                             "AVG_SRVC_CASSA = 13\n"

                             "WORKSTATIONS_PRIMI = 50\n"
                             "WORKSTATIONS_SECONDI = 51\n"
                             "WORKSTATIONS_COFFEE = 52\n"
                             "WORKSTATIONS_CASSA = 53\n"

                             "MAX_PAUSES_PER_DAY = 5\n"
                             "PAUSE_DURATION_NS = 6\n"
                             "PAUSE_PROBABILITY_PERCENT = 7\n"

                             "NOF_WK_SEATS_PRIMI = 20\n"
                             "NOF_WK_SEATS_SECONDI = 21\n"
                             "NOF_WK_SEATS_COFFEE = 22\n"
                             "NOF_WK_SEATS_CASSA = 23\n"

                             "PRICE_PRIMI = 30\n"
                             "PRICE_SECONDI = 31\n"
                             "PRICE_COFFEE = 32\n"

                             "AVG_REFILL_PRIMI = 40\n"
                             "AVG_REFILL_SECONDI = 41\n"
                             "MAX_PORZIONI_PRIMI = 42\n"
                             "MAX_PORZIONI_SECONDI = 43\n"

                             "VARIABILITY_PRIMI = 60\n"
                             "VARIABILITY_SECONDI = 61\n"
                             "VARIABILITY_COFFEE = 62\n"
                             "VARIABILITY_CASSA = 63\n";

  create_temp_config_file(file_content);

  int result = parse_config(TEMP_CONFIG_FILE, &test_config);
  TEST_ASSERT_EQUAL_INT(0, result);

  // general
  TEST_ASSERT_EQUAL_INT(1, test_config.simulation_duration_days);
  TEST_ASSERT_EQUAL_INT(2, test_config.nof_users);
  TEST_ASSERT_EQUAL_INT(3, test_config.nof_workers);
  TEST_ASSERT_EQUAL_INT(4, test_config.nof_table_seats);
  TEST_ASSERT_EQUAL_INT(999, test_config.overload_threshold);

  // service times
  TEST_ASSERT_EQUAL_INT(10, test_config.avg_service_primi);
  TEST_ASSERT_EQUAL_INT(11, test_config.avg_service_secondi);
  TEST_ASSERT_EQUAL_INT(12, test_config.avg_service_coffee);
  TEST_ASSERT_EQUAL_INT(13, test_config.avg_service_cassa);

  // workstations
  TEST_ASSERT_EQUAL_INT(50, test_config.workstations_primi);
  TEST_ASSERT_EQUAL_INT(51, test_config.workstations_secondi);
  TEST_ASSERT_EQUAL_INT(52, test_config.workstations_coffee);
  TEST_ASSERT_EQUAL_INT(53, test_config.workstations_cassa);

  // pauses
  TEST_ASSERT_EQUAL_INT(5, test_config.max_pauses_per_day);
  TEST_ASSERT_EQUAL_INT(6, test_config.pause_duration_ns);
  TEST_ASSERT_EQUAL_INT(7, test_config.pause_probability_percent);

  // queue seats
  TEST_ASSERT_EQUAL_INT(20, test_config.queue_capacity_primi);
  TEST_ASSERT_EQUAL_INT(21, test_config.queue_capacity_secondi);
  TEST_ASSERT_EQUAL_INT(22, test_config.queue_capacity_coffee);
  TEST_ASSERT_EQUAL_INT(23, test_config.queue_capacity_cassa);

  // prices
  TEST_ASSERT_EQUAL_INT(30, test_config.price_primi);
  TEST_ASSERT_EQUAL_INT(31, test_config.price_secondi);
  TEST_ASSERT_EQUAL_INT(32, test_config.price_coffee);

  // refills
  TEST_ASSERT_EQUAL_INT(40, test_config.avg_refill_primi);
  TEST_ASSERT_EQUAL_INT(41, test_config.avg_refill_secondi);
  TEST_ASSERT_EQUAL_INT(42, test_config.max_porzioni_primi);
  TEST_ASSERT_EQUAL_INT(43, test_config.max_porzioni_secondi);

  // variability
  TEST_ASSERT_EQUAL_INT(60, test_config.variability_primi);
  TEST_ASSERT_EQUAL_INT(61, test_config.variability_secondi);
  TEST_ASSERT_EQUAL_INT(62, test_config.variability_coffee);
  TEST_ASSERT_EQUAL_INT(63, test_config.variability_cassa);
}

int main(void) {
  UNITY_BEGIN();

  // parsing
  RUN_TEST(test_file_not_found);
  RUN_TEST(test_parse_valid_file);

  // defaults & overrides
  RUN_TEST(test_default_values_on_empty_file);
  RUN_TEST(test_partial_overrides);
  RUN_TEST(test_actual_default_conf_file);

  // edge cases
  RUN_TEST(test_comments_and_empty_lines);

  // robustness
  RUN_TEST(test_malformed_lines);
  RUN_TEST(test_unknown_keys_ignored);
  RUN_TEST(test_non_numeric_values);
  RUN_TEST(test_random_parameters_parsing);

  // integration
  RUN_TEST(test_parse_all_fields);

  return UNITY_END();
}