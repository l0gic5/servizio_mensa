#include "common/ipc_utils.h"
#include "common/logger.h"

key_t get_project_ipc_key(int project_id) {
  char path[256];

  snprintf(path, sizeof(path), "/tmp/%s", IPC_FILENAME);

  int fd = open(path, O_CREAT | O_RDWR, 0666);
  if (fd < 0) {
    TEST_ERROR;
    return -1;
  }
  close(fd);

  key_t key = ftok(path, project_id);
  if (key == -1) {
    TEST_ERROR;
  }
  return key;
}

/////////////////////
//  SHARED MEMORY  //
/////////////////////

int allocate_shm(size_t size) {
  key_t key = get_project_ipc_key(FTOK_SHM_ID);
  if (key == -1) {
    return -1;
  }

  // IPC_CREAT: Crea se non esiste
  // 0666: Permessi lettura/scrittura per tutti
  int id = shmget(key, size, IPC_CREAT | 0666);
  if (id == -1) {
    TEST_ERROR;
  }
  return id;
}

void *attach_shm(int shm_id) {
  void *ptr = shmat(shm_id, NULL, 0);
  if (ptr == (void *)-1) {
    TEST_ERROR;
    return NULL;
  }
  return ptr;
}

int detach_shm(void *ptr) {
  int res = shmdt(ptr);
  if (res == -1) {
    TEST_ERROR;
  }
  return res;
}

int remove_shm(int shm_id) {
  int res = shmctl(shm_id, IPC_RMID, NULL);
  if (res == -1) {
    TEST_ERROR;
  }
  return res;
}

//////////////////
//  SEMAPHORES  //
//////////////////

int create_sem_set(int num_sems) {
  key_t key = get_project_ipc_key(FTOK_SEM_ID);
  if (key == -1)
    return -1;

  int id = semget(key, num_sems, IPC_CREAT | 0666);
  if (id == -1) {
    TEST_ERROR;
  }
  return id;
}

int init_sem(int sem_id, int sem_num, int value) {
  union semun arg;
  arg.val = value;

  // SETVAL: semaforo n-esimo
  int res = semctl(sem_id, sem_num, SETVAL, arg);
  if (res == -1) {
    TEST_ERROR;
  }
  return res;
}

int sem_wait(int sem_id, int sem_num) {
  struct sembuf sb;
  sb.sem_num = (unsigned short)sem_num;
  // (P operation) --, blocca se 0
  sb.sem_op = -1;
  // evita deadlock
  sb.sem_flg = SEM_UNDO;

  int res = semop(sem_id, &sb, 1);
  if (res == -1) {
    // se arriva un segnale (es. SIGUSR1)
    if (errno != EINTR) {
      TEST_ERROR;
    }
  }
  return res;
}

int sem_signal(int sem_id, int sem_num) {
  struct sembuf sb;
  sb.sem_num = (unsigned short)sem_num;
  // ++ (V operation)
  sb.sem_op = 1;
  sb.sem_flg = SEM_UNDO;

  int res = semop(sem_id, &sb, 1);
  if (res == -1) {
    TEST_ERROR;
  }
  return res;
}

int remove_sem_set(int sem_id) {
  // 0 è ignorato con IPC_RMID
  int res = semctl(sem_id, 0, IPC_RMID);
  if (res == -1) {
    TEST_ERROR;
  }
  return res;
}

//////////////////////
//  MESSAGE QUEUES  //
//////////////////////

int create_msg_queue(void) {
  key_t key = get_project_ipc_key(FTOK_MSG_ID);
  if (key == -1) {
    return -1;
  }

  int id = msgget(key, IPC_CREAT | 0666);
  if (id == -1) {
    TEST_ERROR;
  }
  return id;
}

int send_message(int msg_id, void *msg, size_t size, int flags) {
  // payload = (dimensione tot struct - sizeof(long mtype))
  int res = msgsnd(msg_id, msg, size, flags);
  if (res == -1) {
    // ignora se coda piena e richiesto non bloccante
    if (!(errno == EAGAIN && (flags & IPC_NOWAIT))) {
      TEST_ERROR;
    }
  }
  return res;
}

int receive_message(int msg_id, void *msg, size_t size, long msg_type,
                    int flags) {
  // msgrcv => numero di byte copiati nel buffer mtext
  ssize_t res = msgrcv(msg_id, msg, size, msg_type, flags);

  if (res == -1) {
    // ignora se coda vuota (IPC_NOWAIT) o interruzione segnale
    if (errno != ENOMSG && errno != EINTR) {
      TEST_ERROR;
    }
    return -1;
  }

  return (int)res;
}

int remove_msg_queue(int msg_id) {
  int res = msgctl(msg_id, IPC_RMID, NULL);

  if (res == -1) {
    TEST_ERROR;
  }

  return res;
}