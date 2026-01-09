#include "common/stats.h"
#include "unity.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define TEST_EXPORT_DIR "test_reports"
#define TEST_DAILY_PREFIX "daily"
#define TEST_FINAL_PREFIX "final"

void setUp(void) {
  char cmd[256];
  sprintf(cmd, "rm -rf %s", TEST_EXPORT_DIR);
  system(cmd);
}

void tearDown(void) {
  char cmd[256];
  sprintf(cmd, "rm -rf %s", TEST_EXPORT_DIR);
  system(cmd);
}

void test_process_daily_report_formatting(void) {
  DailyReport report = {.day_number = 1,
                        .daily_users_served = 10,
                        .daily_users_refused = 2,
                        .daily_revenue = 150.50,
                        .daily_plates_primi = 8,
                        .daily_wait_primi = 40.0};

  GlobalStats total = {.total_users_served = 10, .total_revenue = 150.50};

  char *output = process_daily_report(&report, &total);

  TEST_ASSERT_NOT_NULL(output);
  TEST_ASSERT_NOT_NULL(strstr(output, "REPORT GIORNO 1"));
  TEST_ASSERT_NOT_NULL(strstr(output, "150.50"));
  TEST_ASSERT_NOT_NULL(strstr(output, "5.00000"));

  free(output);
}

void test_final_report_division_by_zero(void) {
  GlobalStats stats = {0};

  char *output = process_final_report(&stats, 0);

  TEST_ASSERT_NOT_NULL(output);
  TEST_ASSERT_NOT_NULL(strstr(output, "0.00000 s"));

  free(output);
}

void test_export_daily_csv_creation(void) {
  DailyReport report = {
      .day_number = 5, .daily_users_served = 20, .daily_revenue = 300.0};

  export_daily_stats_to_csv(&report, TEST_EXPORT_DIR, TEST_DAILY_PREFIX,
                            TEST_FINAL_PREFIX, false);

  char final_path[256];
  sprintf(final_path, "%s/%s.csv", TEST_EXPORT_DIR, TEST_FINAL_PREFIX);

  TEST_ASSERT_EQUAL_INT(0, access(final_path, F_OK));

  FILE *f = fopen(final_path, "r");
  char line[1024];
  int line_count = 0;
  while (fgets(line, sizeof(line), f)) {
    line_count++;
  }
  fclose(f);

  TEST_ASSERT_EQUAL_INT(2, line_count);
}

void test_export_path_logic(void) {
  DailyReport report = {.day_number = 1};

  // cartella con slash finale
  export_daily_stats_to_csv(&report, "test_dir/", "prefix", "final", true);
  TEST_ASSERT_EQUAL_INT(0, access("test_dir/prefix-day_1.csv", F_OK));

  // cartella senza slash finale
  export_daily_stats_to_csv(&report, "test_dir_no_slash", "prefix", "final",
                            true);
  TEST_ASSERT_EQUAL_INT(0, access("test_dir_no_slash/prefix-day_1.csv", F_OK));

  system("rm -rf test_dir test_dir_no_slash");
}

void test_export_final_stats(void) {
  GlobalStats stats = {.total_users_served = 100,
                       .total_revenue = 1250.75,
                       .total_plates_primi = 80,
                       .total_leftover_primi = 20};

  export_final_stats_to_csv(&stats, 10, TEST_EXPORT_DIR, TEST_FINAL_PREFIX);

  char path[256];
  sprintf(path, "%s/%s.csv", TEST_EXPORT_DIR, TEST_FINAL_PREFIX);

  FILE *f = fopen(path, "r");
  char content[2048];
  fread(content, 1, sizeof(content), f);
  fclose(f);

  TEST_ASSERT_NOT_NULL(strstr(content, "GLOBALE_FINALE"));
  TEST_ASSERT_NOT_NULL(strstr(content, "1250.75"));
  // calcolo media avanzi (total_leftover / total_days) -> 20 / 10 = 2.00
  TEST_ASSERT_NOT_NULL(strstr(content, "2.00"));
}

void test_csv_header_integrity_on_multiple_appends(void) {
  DailyReport report = {.day_number = 1, .daily_users_served = 5};

  // crea file + header + riga
  export_daily_stats_to_csv(&report, TEST_EXPORT_DIR, "multi", "append_test",
                            false);

  // (giorno 2): appende riga
  report.day_number = 2;
  export_daily_stats_to_csv(&report, TEST_EXPORT_DIR, "multi", "append_test",
                            false);

  char path[256];
  sprintf(path, "%s/append_test.csv", TEST_EXPORT_DIR);

  FILE *f = fopen(path, "r");
  char line[1024];
  int header_count = 0;
  int line_count = 0;
  while (fgets(line, sizeof(line), f)) {
    if (strstr(line, "Tipo,Giorno")) {
      header_count++;
    }
    line_count++;
  }
  fclose(f);

  TEST_ASSERT_EQUAL_INT(1, header_count);
  TEST_ASSERT_EQUAL_INT(3, line_count);
}

void test_export_to_unwritable_directory(void) {
  const char *readonly_dir = "readonly_dir";
  // solo lettura
  mkdir(readonly_dir, 0444);

  DailyReport report = {.day_number = 1};
  export_daily_stats_to_csv(&report, readonly_dir, "test", "final", true);

  rmdir(readonly_dir);
}

void test_averages_with_zero_plates(void) {
  DailyReport report = {
      .day_number = 1, .daily_plates_primi = 0, .daily_wait_primi = 500.0};
  GlobalStats total = {0};

  char *output = process_daily_report(&report, &total);

  TEST_ASSERT_NOT_NULL(strstr(output, "0.00000 s"));
  free(output);
}

void test_recursive_directory_creation(void) {
  const char *deep_path = TEST_EXPORT_DIR "/level1/level2/level3";
  DailyReport report = {.day_number = 1};

  export_daily_stats_to_csv(&report, deep_path, "deep", "final", true);

  char expected_file[256];
  sprintf(expected_file, "%s/deep-day_1.csv", deep_path);

  TEST_ASSERT_EQUAL_INT(0, access(expected_file, F_OK));
}

void test_csv_float_precision(void) {
  DailyReport report = {.day_number = 1, .daily_revenue = 123.4567};
  export_daily_stats_to_csv(&report, TEST_EXPORT_DIR, "prefix", "precision",
                            false);

  char path[256];
  sprintf(path, "%s/precision.csv", TEST_EXPORT_DIR);

  FILE *f = fopen(path, "r");
  char content[1024];
  fread(content, 1, sizeof(content), f);
  fclose(f);

  TEST_ASSERT_NOT_NULL(strstr(content, "123.46"));
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_process_daily_report_formatting);
  RUN_TEST(test_final_report_division_by_zero);
  RUN_TEST(test_export_daily_csv_creation);
  RUN_TEST(test_export_path_logic);
  RUN_TEST(test_export_final_stats);

  RUN_TEST(test_csv_header_integrity_on_multiple_appends);
  RUN_TEST(test_export_to_unwritable_directory);
  RUN_TEST(test_averages_with_zero_plates);
  RUN_TEST(test_recursive_directory_creation);
  RUN_TEST(test_csv_float_precision);

  return UNITY_END();
}