#include "common/config.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEMP_CONFIG_FILE "test_config.tmp"

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
  const char *file_content = "# Commento ignorato\n"
                             "SIM_DURATION = 10\n"
                             "NOF_USERS=50\n"
                             "PRICE_PRIMI =  5 \n";

  create_temp_config_file(file_content);

  int result = parse_config(TEMP_CONFIG_FILE, &test_config);

  TEST_ASSERT_EQUAL_INT(0, result);
  TEST_ASSERT_EQUAL_INT(10, test_config.sim_duration);
  TEST_ASSERT_EQUAL_INT(50, test_config.n_users);
  TEST_ASSERT_EQUAL_INT(5, test_config.price_primi);

  TEST_ASSERT_EQUAL_INT(0, test_config.price_secondi);
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
  TEST_ASSERT_EQUAL_INT(0, test_config.sim_duration);
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

  TEST_ASSERT_EQUAL_INT(0, test_config.sim_duration);
  TEST_ASSERT_EQUAL_INT(0, test_config.n_users);
}

static void test_unknown_keys_ignored(void) {
  const char *file_content = "CHIAVE_INVENTATA = 100\n"
                             "SIM_DURATION = 5\n"
                             "ALTRA_CHIAVE = 999\n";

  create_temp_config_file(file_content);

  int result = parse_config(TEMP_CONFIG_FILE, &test_config);

  TEST_ASSERT_EQUAL_INT(0, result);
  TEST_ASSERT_EQUAL_INT(5, test_config.sim_duration);
}

static void test_non_numeric_values(void) {
  const char *file_content = "SIM_DURATION = trenta\n"
                             "NOF_USERS = 10abc\n";

  create_temp_config_file(file_content);

  parse_config(TEMP_CONFIG_FILE, &test_config);

  TEST_ASSERT_EQUAL_INT(0, test_config.sim_duration);
  // 10abc -> 10
  TEST_ASSERT_EQUAL_INT(10, test_config.n_users);
}

///////////////////
//  INTEGRATION  //
///////////////////

static void test_parse_all_fields(void) {
  const char *file_content = "SIM_DURATION = 1\n"
                             "NOF_USERS = 2\n"
                             "NOF_WORKERS = 3\n"
                             "NOF_TABLE_SEATS = 4\n"

                             "AVG_SRVC_PRIMI = 10\n"
                             "AVG_SRVC_MAIN_COURSE = 11\n"

                             "AVG_SRVC_COFFEE = 12\n"
                             "AVG_SRVC_CASSA = 13\n"

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
                             "MAX_PORZIONI_SECONDI = 43\n";

  create_temp_config_file(file_content);

  int result = parse_config(TEMP_CONFIG_FILE, &test_config);
  TEST_ASSERT_EQUAL_INT(0, result);

  // general
  TEST_ASSERT_EQUAL_INT(1, test_config.sim_duration);
  TEST_ASSERT_EQUAL_INT(2, test_config.n_users);
  TEST_ASSERT_EQUAL_INT(3, test_config.n_workers);
  TEST_ASSERT_EQUAL_INT(4, test_config.table_seats);

  // service times
  TEST_ASSERT_EQUAL_INT(10, test_config.avg_service_primi);
  TEST_ASSERT_EQUAL_INT(11, test_config.avg_service_secondi);
  TEST_ASSERT_EQUAL_INT(12, test_config.avg_service_coffee);
  TEST_ASSERT_EQUAL_INT(13, test_config.avg_service_cassa);

  // queue seats
  TEST_ASSERT_EQUAL_INT(20, test_config.seats_primi);
  TEST_ASSERT_EQUAL_INT(21, test_config.seats_secondi);
  TEST_ASSERT_EQUAL_INT(22, test_config.seats_coffee);
  TEST_ASSERT_EQUAL_INT(23, test_config.seats_cassa);

  // prices
  TEST_ASSERT_EQUAL_INT(30, test_config.price_primi);
  TEST_ASSERT_EQUAL_INT(31, test_config.price_secondi);
  TEST_ASSERT_EQUAL_INT(32, test_config.price_coffee);

  // refills
  TEST_ASSERT_EQUAL_INT(40, test_config.avg_refill_primi);
  TEST_ASSERT_EQUAL_INT(41, test_config.avg_refill_secondi);
  TEST_ASSERT_EQUAL_INT(42, test_config.max_porzioni_primi);
  TEST_ASSERT_EQUAL_INT(43, test_config.max_porzioni_secondi);
}

int main(void) {
  UNITY_BEGIN();

  // parsing
  RUN_TEST(test_file_not_found);
  RUN_TEST(test_parse_valid_file);

  // edge cases
  RUN_TEST(test_comments_and_empty_lines);
  
  // robustness
  RUN_TEST(test_malformed_lines);
  RUN_TEST(test_unknown_keys_ignored);
  RUN_TEST(test_non_numeric_values);

  // integration
  RUN_TEST(test_parse_all_fields);

  return UNITY_END();
}