#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/config.h"
#include "common/menu.h"

#define TEST_MENU_FILE "test_menu.txt"

static void create_dummy_menu_file(const char *filename) {
  FILE *f = fopen(filename, "w");
  if (!f) {
    TEST_FAIL_MESSAGE("Impossibile creare il file dummy per il test");
  }

  // 3 Primi
  fprintf(f, "P|Pasta al Pomodoro\n");
  fprintf(f, "P|Risotto ai Funghi\n");
  fprintf(f, "P|Carbonara\n");

  // 2 Secondi
  fprintf(f, "S|Cotoletta\n");
  fprintf(f, "S|Bistecca\n");

  // 1 Dolce
  fprintf(f, "D|Tiramisu\n");

  // 2 Caffè
  fprintf(f, "C|Espresso\n");
  fprintf(f, "C|Decaffeinato\n");

  fclose(f);
}

void setUp(void) {
  remove(TEST_MENU_FILE);
  srand(42);
}

void tearDown(void) { remove(TEST_MENU_FILE); }

void test_menu_init_file_not_found(void) {
  int res = menu_init("non_existent_file.txt");
  TEST_ASSERT_EQUAL_INT(-1, res);
}

void test_menu_workflow_full_lifecycle(void) {
  create_dummy_menu_file(TEST_MENU_FILE);

  int res = menu_init(TEST_MENU_FILE);
  TEST_ASSERT_EQUAL_INT_MESSAGE(0, res,
                                "menu_init dovrebbe tornare 0 su successo");

  Config config = {0};
  DailyMenu daily = {0};

  // Primi: 2 (disponibili 3)
  // Secondi: 5 (disponibili 2) => saturazione a 2
  // Dolci: 0
  // Caffè: 1 (disponibili 2)
  config.daily_primi_count = 2;
  config.daily_secondi_count = 5;
  config.daily_dolci_count = 0;
  config.daily_caffe_count = 1;

  generate_daily_menu(&daily, &config);

  TEST_ASSERT_EQUAL_INT_MESSAGE(
      2, daily.primi_count, "Dovrebbe aver selezionato esattamente 2 primi");
  for (int i = 0; i < daily.primi_count; i++) {
    TEST_ASSERT_EQUAL_CHAR('P', daily.daily_primi[i].type);
    TEST_ASSERT_NOT_EQUAL(0, strlen(daily.daily_primi[i].name));
  }

  TEST_ASSERT_EQUAL_INT_MESSAGE(
      2, daily.secondi_count,
      "Non può generare più secondi di quelli nel file (2)");
  TEST_ASSERT_EQUAL_CHAR('S', daily.daily_secondi[0].type);

  TEST_ASSERT_EQUAL_INT_MESSAGE(0, daily.dolci_count,
                                "Richiesti 0 dolci, ottenuti > 0");

  TEST_ASSERT_EQUAL_INT_MESSAGE(1, daily.caffe_count, "Richiesto 1 caffè");
  TEST_ASSERT_EQUAL_CHAR('C', daily.daily_caffe[0].type);

  menu_destroy();
}

void test_menu_parsing_empty_lines_and_garbage(void) {
  FILE *f = fopen(TEST_MENU_FILE, "w");
  fprintf(f, "P|Pasta\n");
  fprintf(f, "\n");
  fprintf(f, "GarbageLine\n");
  fprintf(f, "|NoType\n");
  fprintf(f, "S|Carne\n");
  fclose(f);

  if (menu_init(TEST_MENU_FILE) == 0) {
    Config cfg = {0};
    cfg.daily_primi_count = 10;
    cfg.daily_secondi_count = 10;
    DailyMenu menu = {0};

    generate_daily_menu(&menu, &cfg);

    bool found_pasta = false;
    bool found_carne = false;

    for (int i = 0; i < menu.primi_count; i++) {
      if (strstr(menu.daily_primi[i].name, "Pasta"))
        found_pasta = true;
    }
    for (int i = 0; i < menu.secondi_count; i++) {
      if (strstr(menu.daily_secondi[i].name, "Carne"))
        found_carne = true;
    }

    TEST_ASSERT_TRUE_MESSAGE(found_pasta, "Parsing fallito: Pasta non trovata");
    TEST_ASSERT_TRUE_MESSAGE(found_carne, "Parsing fallito: Carne non trovata");

    menu_destroy();
  }
}

void test_large_file_reallocation(void) {
  FILE *f = fopen(TEST_MENU_FILE, "w");

  for (int i = 0; i < 25; i++) {
    fprintf(f, "P|PrimoPiattoNumero%d\n", i);
  }
  fclose(f);

  int res = menu_init(TEST_MENU_FILE);
  TEST_ASSERT_EQUAL_INT(0, res);

  Config cfg = {0};
  cfg.daily_primi_count = 25;
  DailyMenu menu = {0};

  generate_daily_menu(&menu, &cfg);

  TEST_ASSERT_EQUAL_INT(25, menu.primi_count);

  TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(menu.daily_primi[0].name, "PrimoPiattoNumero"),
      "La stringa recuperata non contiene il prefisso atteso, memoria "
      "possibilmente corrotta");

  menu_destroy();
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_menu_init_file_not_found);

  RUN_TEST(test_menu_workflow_full_lifecycle);

  RUN_TEST(test_menu_parsing_empty_lines_and_garbage);
  RUN_TEST(test_large_file_reallocation);

  return UNITY_END();
}