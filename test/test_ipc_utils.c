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
 *
 * Se un test precedente è crashato (segfault) prima di tearDown,
 * le risorse IPC rimangono nel sistema.
 *
 * Questa funzione prova a collegarsi usando le chiavi note e
 * rimuove tutto prima di iniziare i nuovi test.
 */
static void clean_stale_ipc_resources(void) {
  // shared memory
  key_t key = get_project_ipc_key(FTOK_SHM_ID);
  if (key != -1) {
    // size = 0 => "dammi quello esistente"
    int id = shmget(key, 0, 0666);
    if (id != -1) {
      shmctl(id, IPC_RMID, NULL);
      printf("[Test Setup] Rimossa SHM residua ID: %d\n", id);
    }
  }

  // semaphores
  key = get_project_ipc_key(FTOK_SEM_ID);
  if (key != -1) {
    // 0 nsems per lookup
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
  // ID produce una chiave valida (> 0) ?
  key_t key_shm = get_project_ipc_key(FTOK_SHM_ID);
  key_t key_sem = get_project_ipc_key(FTOK_SEM_ID);
  key_t key_msg = get_project_ipc_key(FTOK_MSG_ID);

  TEST_ASSERT_NOT_EQUAL_INT(-1, key_shm);
  TEST_ASSERT_NOT_EQUAL_INT(-1, key_sem);
  TEST_ASSERT_NOT_EQUAL_INT(-1, key_msg);

  TEST_ASSERT_GREATER_THAN(0, key_shm);
  TEST_ASSERT_GREATER_THAN(0, key_sem);
  TEST_ASSERT_GREATER_THAN(0, key_msg);
}

static void test_ipc_keys_are_distinct(void) {
  // ftok: chiavi diverse per ID diversi?
  key_t key_shm = get_project_ipc_key(FTOK_SHM_ID);
  key_t key_sem = get_project_ipc_key(FTOK_SEM_ID);
  key_t key_msg = get_project_ipc_key(FTOK_MSG_ID);

  TEST_ASSERT_NOT_EQUAL_INT(key_shm, key_sem);
  TEST_ASSERT_NOT_EQUAL_INT(key_shm, key_msg);
  TEST_ASSERT_NOT_EQUAL_INT(key_sem, key_msg);
}

//////////////////////////
//  INTEGRATION CHECKS  //
//////////////////////////

static void test_all_resources_coexist(void) {
  g_shm_id = allocate_shm(1024);
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

  // allocation
  g_shm_id = allocate_shm(size);
  TEST_ASSERT_NOT_EQUAL_INT(-1, g_shm_id);

  // attach
  int *data = (int *)attach_shm(g_shm_id);
  TEST_ASSERT_NOT_NULL(data);

  // write data
  *data = 42;
  TEST_ASSERT_EQUAL_INT(42, *data);

  // detach
  int res = detach_shm(data);
  TEST_ASSERT_EQUAL_INT(0, res);

  // remove
  res = remove_shm(g_shm_id);
  TEST_ASSERT_EQUAL_INT(0, res);

  g_shm_id = -1;
}

///////////////////////
//  SEMAPHORE TESTS  //
///////////////////////

static void test_sem_set_creation(void) {
  g_sem_id = create_sem_set(2);
  TEST_ASSERT_NOT_EQUAL_INT(-1, g_sem_id);
}

static void test_sem_logic(void) {
  g_sem_id = create_sem_set(1);
  TEST_ASSERT_NOT_EQUAL_INT(-1, g_sem_id);

  int res = init_sem(g_sem_id, 0, 1);
  TEST_ASSERT_EQUAL_INT(0, res);

  // wait(P) => decrementa a 0, non blocca perché era 1
  res = sem_wait(g_sem_id, 0);
  TEST_ASSERT_EQUAL_INT(0, res);

  // signal(V) => incrementa a 1
  res = sem_signal(g_sem_id, 0);
  TEST_ASSERT_EQUAL_INT(0, res);

  // wait(P) => decrementa a 0
  res = sem_wait(g_sem_id, 0);
  TEST_ASSERT_EQUAL_INT(0, res);
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

  // send
  int res = send_message(g_msg_id, &send_buf, payload_size, 0);
  TEST_ASSERT_EQUAL_INT(0, res);

  struct test_msg_buf rcv_buf;
  memset(&rcv_buf, 0, sizeof(rcv_buf));

  // receive
  int bytes = receive_message(g_msg_id, &rcv_buf, payload_size, 1, 0);

  TEST_ASSERT_EQUAL_INT(payload_size, bytes);
  TEST_ASSERT_EQUAL_STRING("Hello Unity", rcv_buf.mtext);

  // remove
  res = remove_msg_queue(g_msg_id);
  TEST_ASSERT_EQUAL_INT(0, res);
  g_msg_id = -1;
}

static void test_msg_queue_nowait(void) {
  g_msg_id = create_msg_queue();
  TEST_ASSERT_NOT_EQUAL_INT(-1, g_msg_id);

  struct test_msg_buf rcv_buf;

  // empty queue con IPC_NOWAIT
  int bytes =
      receive_message(g_msg_id, &rcv_buf, sizeof(rcv_buf.mtext), 1, IPC_NOWAIT);

  // deve fallire con -1 e errno ENOMSG
  TEST_ASSERT_EQUAL_INT(-1, bytes);
  TEST_ASSERT_EQUAL_INT(ENOMSG, errno);
}

//////////////////////////////
//  ADVANCED MULTI-PROCESS  //
//////////////////////////////

static void test_shm_across_processes(void) {
  size_t size = sizeof(int);
  g_shm_id = allocate_shm(size);
  TEST_ASSERT_NOT_EQUAL_INT(-1, g_shm_id);

  int *p_data = (int *)attach_shm(g_shm_id);
  *p_data = 100;

  pid_t pid = fork();
  TEST_ASSERT_NOT_EQUAL_INT(-1, pid);

  if (pid == 0) {
    // FIGLIO
    // stesso segmento tramite la chiave condivisa
    // attach_shm chiama get_project_ipc_key(FTOK_SHM_ID) internamente
    int *c_data = (int *)attach_shm(g_shm_id);
    if (*c_data == 100) {
      *c_data = 200;
      exit(0);
    } else {
      exit(1);
    }
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
  // inizializza a 0 (rosso)
  init_sem(g_sem_id, 0, 0);

  pid_t pid = fork();
  TEST_ASSERT_NOT_EQUAL_INT(-1, pid);

  if (pid == 0) {
    // FIGLIO
    // bloccato finché il padre non fa signal
    sem_wait(g_sem_id, 0);
    exit(0);
  } else {
    // PADRE

    // 100ms
    usleep(100000);

    // figlio ancora vivo? (status 0 in WNOHANG)
    int status;
    int res = waitpid(pid, &status, WNOHANG);
    TEST_ASSERT_EQUAL_INT(0, res);

    // sblocca il figlio
    sem_signal(g_sem_id, 0);

    // figlio deve terminare
    res = waitpid(pid, &status, 0);
    TEST_ASSERT_EQUAL_INT(pid, res);
    TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
  }
}

static void test_msg_queue_type_filtering(void) {
  g_msg_id = create_msg_queue();
  struct test_msg_buf msg;
  size_t len = sizeof(msg.mtext);

  // 1. messaggio priorità bassa (type 1)
  msg.mtype = 1;
  strcpy(msg.mtext, "Low Priority");
  send_message(g_msg_id, &msg, len, 0);

  // 2. messaggio priorità alta (type 2)
  msg.mtype = 2;
  strcpy(msg.mtext, "High Priority");
  send_message(g_msg_id, &msg, len, 0);

  struct test_msg_buf rcv;

  // 3. leggo SOLO type 2 (salto type 1 => in testa)
  receive_message(g_msg_id, &rcv, len, 2, 0);
  TEST_ASSERT_EQUAL_INT(2, rcv.mtype);
  TEST_ASSERT_EQUAL_STRING("High Priority", rcv.mtext);

  // 4. leggo rimanente (type 1)
  receive_message(g_msg_id, &rcv, len, 1, 0);
  TEST_ASSERT_EQUAL_INT(1, rcv.mtype);
  TEST_ASSERT_EQUAL_STRING("Low Priority", rcv.mtext);
}

int main(void) {
  // PULIZIA PREVENTIVA
  clean_stale_ipc_resources();

  UNITY_BEGIN();

  // KEY & COEXISTENCE TESTS
  RUN_TEST(test_get_project_ipc_key_validity);
  RUN_TEST(test_ipc_keys_are_distinct);
  RUN_TEST(test_all_resources_coexist);

  // UNIT TESTS
  RUN_TEST(test_shm_lifecycle);
  RUN_TEST(test_sem_set_creation);
  RUN_TEST(test_sem_logic);
  RUN_TEST(test_msg_queue_lifecycle);
  RUN_TEST(test_msg_queue_nowait);

  // MULTI-PROCESS SCENARIOS
  RUN_TEST(test_shm_across_processes);
  RUN_TEST(test_sem_blocking_with_fork);
  RUN_TEST(test_msg_queue_type_filtering);

  return UNITY_END();
}