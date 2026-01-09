#include "common/names.h"
#include "unity.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define TEST_DATA_DIR "data"
#define REAL_DATA_BACKUP "data_backup_temp"

#define TEST_FILE_M "data/nomi_maschili.txt"
#define TEST_FILE_F "data/nomi_femminili.txt"
#define TEST_FILE_S "data/cognomi.txt"

/**
 * @brief Helper per creare file di testo dummy
 */
static void create_dummy_file(const char *path, const char *content) {
  FILE *f = fopen(path, "w");
  if (f) {
    fprintf(f, "%s", content);
    fclose(f);
  } else {
    printf("ERRORE: Impossibile creare file test %s\n", path);
  }
}

/**
 * @brief Setup: Salva la cartella 'data' reale e ne crea una fake.
 */
void setUp(void) {
  struct stat st;

  if (stat(TEST_DATA_DIR, &st) == 0 && S_ISDIR(st.st_mode)) {
    if (rename(TEST_DATA_DIR, REAL_DATA_BACKUP) != 0) {
      printf("ERRORE CRITICO: Impossibile fare backup dei dati reali!\n");
      exit(1);
    }
  }

  // cartella sandbox per il test
  mkdir(TEST_DATA_DIR, 0755);

  create_dummy_file(TEST_FILE_M, "Mario\nLuigi\nPaolo");
  create_dummy_file(TEST_FILE_F, "Maria\nLucia\nAnna");
  create_dummy_file(TEST_FILE_S, "Rossi\nBianchi\nVerdi");
}

/**
 * @brief Teardown: Pulisce la memoria, elimina i dati finti e ripristina quelli
 * veri.
 */
void tearDown(void) {
  names_destroy();

  char cmd[256];
  sprintf(cmd, "rm -rf %s", TEST_DATA_DIR);
  system(cmd);

  struct stat st;
  if (stat(REAL_DATA_BACKUP, &st) == 0 && S_ISDIR(st.st_mode)) {

    if (rename(REAL_DATA_BACKUP, TEST_DATA_DIR) != 0) {
      perror("ERRORE GRAVE: Impossibile ripristinare i dati originali!");
    }
  }
}

void test_names_init_success(void) {
  int res = names_init();
  TEST_ASSERT_EQUAL_INT(0, res);
}

void test_names_init_failure_missing_file(void) {
  remove(TEST_FILE_M);

  int res = names_init();
  TEST_ASSERT_EQUAL_INT(-1, res);
}

void test_get_identity_structure_responsabile(void) {
  names_init();
  char *id = get_random_identity(ROLE_RESPONSABILE);

  TEST_ASSERT_NOT_NULL(id);
  TEST_ASSERT_NOT_NULL(strstr(id, "RESPONSABILE"));
  TEST_ASSERT_NOT_NULL(strstr(id, "MARINA"));

  free(id);
}

void test_double_initialization_safety(void) {
  TEST_ASSERT_EQUAL_INT(0, names_init());

  int res = names_init();
  TEST_ASSERT_EQUAL_INT(0, res);

  char *id = get_random_identity(ROLE_UTENTE);
  TEST_ASSERT_NOT_NULL(id);
  free(id);
}

void test_get_identity_structure_operatore(void) {
  names_init();
  char *id = get_random_identity(ROLE_OPERATORE);

  TEST_ASSERT_NOT_NULL(id);
  TEST_ASSERT_NOT_NULL(strstr(id, "[OPERATORE]"));
  TEST_ASSERT_NOT_NULL(strchr(id, '.'));

  free(id);
}

void test_get_identity_structure_utente(void) {
  names_init();
  char *id = get_random_identity(ROLE_UTENTE);

  TEST_ASSERT_NOT_NULL(id);
  TEST_ASSERT_NOT_NULL(strstr(id, "[UTENTE]"));

  free(id);
}

void test_get_identity_randomness_and_loading(void) {
  names_init();

  for (int i = 0; i < 10; i++) {
    char *id = get_random_identity(ROLE_UTENTE);
    TEST_ASSERT_NOT_NULL(id);
    bool has_valid_surname =
        (strstr(id, "Rossi") || strstr(id, "Bianchi") || strstr(id, "Verdi"));

    TEST_ASSERT_TRUE_MESSAGE(
        has_valid_surname,
        "Il cognome generato non è tra quelli caricati nel test");
    free(id);
  }
}

void test_get_identity_structure_cassa(void) {
  names_init();
  char *id = get_random_identity(ROLE_CASSA);

  TEST_ASSERT_NOT_NULL(id);
  TEST_ASSERT_NOT_NULL(strstr(id, "[CASSA]"));
  TEST_ASSERT_NOT_NULL(strchr(id, '.'));

  free(id);
}

void test_fallback_on_empty_files(void) {
  create_dummy_file(TEST_FILE_M, "");
  create_dummy_file(TEST_FILE_F, "");
  create_dummy_file(TEST_FILE_S, "");

  names_init();

  char *id = get_random_identity(ROLE_UTENTE);

  TEST_ASSERT_NOT_NULL(id);
  TEST_ASSERT_NOT_NULL(strstr(id, "Generico"));
  TEST_ASSERT_NOT_NULL(strstr(id, "Rossi"));

  free(id);
}

void test_names_destroy_idempotency(void) {
  names_init();

  names_destroy();
  names_destroy();

  TEST_PASS();
}

void test_large_file_loading(void) {
  FILE *f = fopen(TEST_FILE_S, "w");
  for (int i = 0; i < 500; i++) {
    fprintf(f, "Cognome%d\n", i);
  }
  fclose(f);

  TEST_ASSERT_EQUAL_INT(0, names_init());

  char *id = get_random_identity(ROLE_UTENTE);
  TEST_ASSERT_NOT_NULL(id);
  free(id);
}

void test_parsing_files_with_empty_lines(void) {
  create_dummy_file(TEST_FILE_M, "Mario\n\n\nLuigi\n\n");
  create_dummy_file(TEST_FILE_S, "Rossi\n");

  TEST_ASSERT_EQUAL_INT(0, names_init());

  for (int i = 0; i < 20; i++) {
    char *id = get_random_identity(ROLE_UTENTE);
    TEST_ASSERT_NOT_NULL(id);

    TEST_ASSERT_TRUE(strlen(id) > 10);

    free(id);
  }
}

void test_long_name_truncation_safety(void) {
  char long_name[201];
  memset(long_name, 'A', 200);
  long_name[200] = '\0';

  create_dummy_file(TEST_FILE_M, long_name);

  names_init();
  char *id = get_random_identity(ROLE_UTENTE);

  TEST_ASSERT_NOT_NULL(id);
  TEST_ASSERT_TRUE(strlen(id) < 200);

  free(id);
}

void test_invalid_role_handling(void) {
  names_init();

  char *id = get_random_identity((PersonRole)999);

  TEST_ASSERT_NOT_NULL(id);
  TEST_ASSERT_NOT_NULL(strstr(id, "[?]"));

  free(id);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_names_init_success);
  RUN_TEST(test_names_init_failure_missing_file);
  RUN_TEST(test_double_initialization_safety);

  RUN_TEST(test_get_identity_structure_responsabile);
  RUN_TEST(test_get_identity_structure_operatore);
  RUN_TEST(test_get_identity_structure_utente);
  RUN_TEST(test_get_identity_randomness_and_loading);
  RUN_TEST(test_get_identity_structure_cassa);

  RUN_TEST(test_parsing_files_with_empty_lines);
  RUN_TEST(test_long_name_truncation_safety);

  RUN_TEST(test_fallback_on_empty_files);
  RUN_TEST(test_names_destroy_idempotency);
  RUN_TEST(test_large_file_loading);

  RUN_TEST(test_invalid_role_handling);

  return UNITY_END();
}