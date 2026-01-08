#include "common/ipc_utils.h"
#include "unity.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int g_shm_id = -1;
static int g_sem_id = -1;
static int g_msg_id = -1;

struct test_msg_buf {
  long mtype;
  char mtext[64];
};

//////////////////////////
//  CLEANUP PREVENTIVO  //
//////////////////////////

/**
 * @brief Rimuove risorse IPC residue da run precedenti crashati.
 */
static void clean_stale_ipc_resources(void) {
  // shared memory (provo con le chiavi standard)
  int proj_ids[] = {FTOK_SHM_ID, FTOK_SHM_ROLES_ID};

  for (int i = 0; i < 2; i++) {
    key_t key = get_project_ipc_key(proj_ids[i]);
    if (key != -1) {
      int id = shmget(key, 0, 0666);
      if (id != -1) {
        shmctl(id, IPC_RMID, NULL);
        printf("[Test Setup] Rimossa SHM residua ID: %d (proj: %c)\n", id,
               proj_ids[i]);
      }
    }
  }

  // semaphores
  key_t key = get_project_ipc_key(FTOK_SEM_ID);
  if (key != -1) {
    int id = semget(key, 0, 0666);
    if (id != -1) {
      semctl(id, 0, IPC_RMID);
      printf("[Test Setup] Rimosso SemSet residuo ID: %d\n", id);
    }
  }

  // message queues
  key = get_project_ipc_key(FTOK_MSG_ID);
  if (key != -1) {
    int id = msgget(key, 0666);
    if (id != -1) {
      msgctl(id, IPC_RMID, NULL);
      printf("[Test Setup] Rimossa MsgQueue residua ID: %d\n", id);
    }
  }
}

////////////////////////////
//  FUNZIONI DI SUPPORTO  //
////////////////////////////

void setUp(void) {
  g_shm_id = -1;
  g_sem_id = -1;
  g_msg_id = -1;
}

void tearDown(void) {
  if (g_shm_id != -1) {
    remove_shm(g_shm_id);
    g_shm_id = -1;
  }
  if (g_sem_id != -1) {
    remove_sem_set(g_sem_id);
    g_sem_id = -1;
  }
  if (g_msg_id != -1) {
    remove_msg_queue(g_msg_id);
    g_msg_id = -1;
  }
}

/////////////////
//  KEY TESTS  //
/////////////////

static void test_get_project_ipc_key_validity(void) {
  key_t key_shm = get_project_ipc_key(FTOK_SHM_ID);
  key_t key_sem = get_project_ipc_key(FTOK_SEM_ID);
  key_t key_msg = get_project_ipc_key(FTOK_MSG_ID);
  key_t key_roles = get_project_ipc_key(FTOK_SHM_ROLES_ID);

  TEST_ASSERT_GREATER_THAN(0, key_shm);
  TEST_ASSERT_GREATER_THAN(0, key_sem);
  TEST_ASSERT_GREATER_THAN(0, key_msg);
  TEST_ASSERT_GREATER_THAN(0, key_roles);
}

static void test_ipc_keys_are_distinct(void) {
  key_t key_shm = get_project_ipc_key(FTOK_SHM_ID);
  key_t key_roles = get_project_ipc_key(FTOK_SHM_ROLES_ID);
  key_t key_sem = get_project_ipc_key(FTOK_SEM_ID);

  TEST_ASSERT_NOT_EQUAL_INT(key_shm, key_roles);
  TEST_ASSERT_NOT_EQUAL_INT(key_shm, key_sem);
}

//////////////////////////
//  INTEGRATION CHECKS  //
//////////////////////////

static void test_all_resources_coexist(void) {
  g_shm_id = allocate_shm(1024, FTOK_SHM_ID);
  TEST_ASSERT_NOT_EQUAL_INT(-1, g_shm_id);

  g_sem_id = create_sem_set(1);
  TEST_ASSERT_NOT_EQUAL_INT(-1, g_sem_id);

  g_msg_id = create_msg_queue();
  TEST_ASSERT_NOT_EQUAL_INT(-1, g_msg_id);

  TEST_PASS();
}

/////////////////
//  SHM TESTS  //
/////////////////

static void test_shm_lifecycle(void) {
  size_t size = 1024;

  // passo FTOK_SHM_ID
  g_shm_id = allocate_shm(size, FTOK_SHM_ID);
  TEST_ASSERT_NOT_EQUAL_INT(-1, g_shm_id);

  int *data = (int *)attach_shm(g_shm_id);
  TEST_ASSERT_NOT_NULL(data);

  *data = 42;
  TEST_ASSERT_EQUAL_INT(42, *data);

  TEST_ASSERT_EQUAL_INT(0, detach_shm(data));
  TEST_ASSERT_EQUAL_INT(0, remove_shm(g_shm_id));
  g_shm_id = -1;
}

static void test_multiple_shm_separation(void) {
  int shm1 = allocate_shm(sizeof(int), FTOK_SHM_ID);
  int shm2 = allocate_shm(sizeof(int), FTOK_SHM_ROLES_ID);

  TEST_ASSERT_NOT_EQUAL_INT(-1, shm1);
  TEST_ASSERT_NOT_EQUAL_INT(-1, shm2);
  // devono essere diversi
  TEST_ASSERT_NOT_EQUAL_INT(shm1, shm2);

  int *p1 = (int *)attach_shm(shm1);
  int *p2 = (int *)attach_shm(shm2);

  *p1 = 100;
  *p2 = 200;

  // la modifica su p1 non deve toccare p2
  TEST_ASSERT_EQUAL_INT(100, *p1);
  TEST_ASSERT_EQUAL_INT(200, *p2);

  detach_shm(p1);
  detach_shm(p2);
  remove_shm(shm1);
  remove_shm(shm2);
}

static void test_detach_shm_null(void) {
  int res = detach_shm(NULL);
  TEST_ASSERT_EQUAL_INT(0, res);
}

///////////////////////
//  SEMAPHORE TESTS  //
///////////////////////

static void test_sem_set_creation(void) {
  g_sem_id = create_sem_set(2);
  TEST_ASSERT_NOT_EQUAL_INT(-1, g_sem_id);
}

static void test_init_sem_value(void) {
  g_sem_id = create_sem_set(1);

  init_sem(g_sem_id, 0, 5);

  int val = semctl(g_sem_id, 0, GETVAL);
  TEST_ASSERT_EQUAL_INT(5, val);
}

static void test_sem_logic(void) {
  g_sem_id = create_sem_set(1);
  init_sem(g_sem_id, 0, 1);

  // 1 -> 0
  TEST_ASSERT_EQUAL_INT(0, sem_wait(g_sem_id, 0));
  // 0 -> 1
  TEST_ASSERT_EQUAL_INT(0, sem_signal(g_sem_id, 0));
  // 1 -> 0
  TEST_ASSERT_EQUAL_INT(0, sem_wait(g_sem_id, 0));
}

static void test_sem_recreation_on_size_mismatch(void) {
  g_sem_id = create_sem_set(1);
  TEST_ASSERT_NOT_EQUAL_INT(-1, g_sem_id);

  int new_id = create_sem_set(5);

  TEST_ASSERT_NOT_EQUAL_INT(-1, new_id);

  struct semid_ds buf;
  union semun arg;
  arg.buf = &buf;
  semctl(new_id, 0, IPC_STAT, arg);

  TEST_ASSERT_EQUAL_INT(5, buf.sem_nsems);

  g_sem_id = new_id;
}

///////////////////////
//  MSG QUEUE TESTS  //
///////////////////////

static void test_msg_queue_lifecycle(void) {
  g_msg_id = create_msg_queue();
  TEST_ASSERT_NOT_EQUAL_INT(-1, g_msg_id);

  struct test_msg_buf send_buf;
  send_buf.mtype = 1;
  strcpy(send_buf.mtext, "Hello Unity");
  size_t payload_size = sizeof(send_buf.mtext);

  int res = send_message(g_msg_id, &send_buf, payload_size, 0);
  TEST_ASSERT_EQUAL_INT(0, res);

  struct test_msg_buf rcv_buf;
  memset(&rcv_buf, 0, sizeof(rcv_buf));

  int bytes = receive_message(g_msg_id, &rcv_buf, payload_size, 1, 0);
  TEST_ASSERT_EQUAL_INT(payload_size, bytes);
  TEST_ASSERT_EQUAL_STRING("Hello Unity", rcv_buf.mtext);

  remove_msg_queue(g_msg_id);
  g_msg_id = -1;
}

static void test_msg_queue_nowait(void) {
  g_msg_id = create_msg_queue();
  struct test_msg_buf rcv_buf;
  int bytes =
      receive_message(g_msg_id, &rcv_buf, sizeof(rcv_buf.mtext), 1, IPC_NOWAIT);
  TEST_ASSERT_EQUAL_INT(-1, bytes);
  TEST_ASSERT_EQUAL_INT(ENOMSG, errno);
}

static void test_msg_queue_full_nowait(void) {
  g_msg_id = create_msg_queue();

  struct msqid_ds buf;
  msgctl(g_msg_id, IPC_STAT, &buf);
  // limit: 100 bytes
  buf.msg_qbytes = 100;
  msgctl(g_msg_id, IPC_SET, &buf);

  struct test_msg_buf msg;
  msg.mtype = 1;
  // 60 bytes
  memset(msg.mtext, 'A', 60);

  // primo invio: OK (60 < 100)
  int res = send_message(g_msg_id, &msg, 60, IPC_NOWAIT);
  TEST_ASSERT_EQUAL_INT(0, res);

  // secondo invio: FULL (60+60 > 100) => IPC_NOWAIT deve fallire
  res = send_message(g_msg_id, &msg, 60, IPC_NOWAIT);
  TEST_ASSERT_EQUAL_INT(-1, res);
  TEST_ASSERT_EQUAL_INT(EAGAIN, errno);
}

//////////////////////////////
//  ADVANCED MULTI-PROCESS  //
//////////////////////////////

static void test_shm_across_processes(void) {
  size_t size = sizeof(int);

  g_shm_id = allocate_shm(size, FTOK_SHM_ID);
  TEST_ASSERT_NOT_EQUAL_INT(-1, g_shm_id);

  int *p_data = (int *)attach_shm(g_shm_id);
  *p_data = 100;

  pid_t pid = fork();
  if (pid == 0) {
    // FIGLIO
    // attach_shm userà shmat, ma per riottenere l'ID in un processo disgiunto
    // (es. exec) servirebbe allocate_shm.

    if (*p_data == 100) {
      *p_data = 200;
      exit(0);
    }
    exit(1);
  } else {
    // PADRE
    int status;
    waitpid(pid, &status, 0);
    TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
    TEST_ASSERT_EQUAL_INT(200, *p_data);
    detach_shm(p_data);
  }
}

static void test_sem_blocking_with_fork(void) {
  g_sem_id = create_sem_set(1);
  init_sem(g_sem_id, 0, 0);

  pid_t pid = fork();
  if (pid == 0) {
    // blocca finché padre non fa signal
    sem_wait(g_sem_id, 0);
    exit(0);
  } else {
    usleep(100000);
    int status;
    TEST_ASSERT_EQUAL_INT(0, waitpid(pid, &status, WNOHANG));
    sem_signal(g_sem_id, 0);
    waitpid(pid, &status, 0);
    TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
  }
}

static void test_msg_queue_type_filtering(void) {
  g_msg_id = create_msg_queue();
  struct test_msg_buf msg;
  size_t len = sizeof(msg.mtext);

  msg.mtype = 1;
  strcpy(msg.mtext, "Low");
  send_message(g_msg_id, &msg, len, 0);
  msg.mtype = 2;
  strcpy(msg.mtext, "High");
  send_message(g_msg_id, &msg, len, 0);

  struct test_msg_buf rcv;
  receive_message(g_msg_id, &rcv, len, 2, 0);
  TEST_ASSERT_EQUAL_INT(2, rcv.mtype);

  receive_message(g_msg_id, &rcv, len, 1, 0);
  TEST_ASSERT_EQUAL_INT(1, rcv.mtype);
}

static void test_sem_mutex_logic(void) {
  // MUTEX = semaforo binario inizializzato a 1
  g_sem_id = create_sem_set(1);
  init_sem(g_sem_id, 0, 1);

  sem_mutex_acquire(g_sem_id, 0);

  // verifica che il semaforo sia a 0 (bloccato)
  int val = semctl(g_sem_id, 0, GETVAL);
  TEST_ASSERT_EQUAL_INT(0, val);

  sem_mutex_release(g_sem_id, 0);

  // verifica che il semaforo sia tornato a 1 (libero)
  val = semctl(g_sem_id, 0, GETVAL);
  TEST_ASSERT_EQUAL_INT(1, val);
}

static void test_sem_mutex_contention(void) {
  g_sem_id = create_sem_set(1);
  init_sem(g_sem_id, 0, 1);

  pid_t pid = fork();
  if (pid == 0) {
    // FIGLIO
    sem_mutex_acquire(g_sem_id, 0);
    // 50ms
    usleep(50000);
    sem_mutex_release(g_sem_id, 0);
    exit(0);
  } else {
    // PADRE
    // 10ms
    usleep(10000);

    sem_mutex_acquire(g_sem_id, 0);

    int status;
    // pulisce lo zombie
    waitpid(pid, &status, 0);

    sem_mutex_release(g_sem_id, 0);
    TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
  }
}

int main(void) {
  clean_stale_ipc_resources();
  UNITY_BEGIN();

  RUN_TEST(test_get_project_ipc_key_validity);
  RUN_TEST(test_ipc_keys_are_distinct);
  RUN_TEST(test_all_resources_coexist);

  RUN_TEST(test_shm_lifecycle);
  RUN_TEST(test_multiple_shm_separation);
  RUN_TEST(test_detach_shm_null);

  RUN_TEST(test_sem_set_creation);
  RUN_TEST(test_init_sem_value);
  RUN_TEST(test_sem_logic);
  RUN_TEST(test_sem_recreation_on_size_mismatch);

  RUN_TEST(test_msg_queue_lifecycle);
  RUN_TEST(test_msg_queue_nowait);
  RUN_TEST(test_msg_queue_full_nowait);

  RUN_TEST(test_shm_across_processes);
  RUN_TEST(test_sem_blocking_with_fork);
  RUN_TEST(test_msg_queue_type_filtering);
  RUN_TEST(test_sem_mutex_logic);
  RUN_TEST(test_sem_mutex_contention);

  return UNITY_END();
}