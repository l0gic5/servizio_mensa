#ifndef TYPES_H
#define TYPES_H

#include <sys/types.h>

#define MAX_BUFFER_SIZE 256

// coda (mtype)
#define MSG_TYPE_ORDER_PRIMI 1
#define MSG_TYPE_ORDER_SECONDI 2
#define MSG_TYPE_ORDER_CAFFE 3
#define MSG_TYPE_PAYMENT 4

// semafori (code utenti)
#define SEM_INDEX_SEATS_PRIMI 0
#define SEM_INDEX_SEATS_SECONDI 1
#define SEM_INDEX_SEATS_CAFFE 2
#define SEM_INDEX_SEATS_CASSA 3
#define SEM_INDEX_TICKET_READER 4

// semafori (risorse)
#define SEM_INDEX_TABLES 5
#define SEM_INDEX_MUTEX_STATS 6
#define SEM_INDEX_OUTPUT 7
#define SEM_INDEX_BARRIER 8

// semafori (operatori)
#define SEM_OPERATORS_PRIMI 9
#define SEM_OPERATORS_SECONDI 10
#define SEM_OPERATORS_CAFFE 11
#define SEM_OPERATORS_CASSA 12

#define SEM_INDEX_DAY_CHANGE 13

#define MAX_GROUPS 100
#define SEM_GROUP_BARRIER_BASE 14

#define TOTAL_SEMS (14 + MAX_GROUPS)
#define MAX_WORKERS 100

typedef struct worker_config {
  // indice worker -> enum OpType
  char worker_names[MAX_WORKERS][64];
  int worker_roles[MAX_WORKERS];
  int total_workers_count;

  volatile time_t strike_end_times[MAX_WORKERS];
  volatile int current_day;

  int active_primi;
  int active_secondi;
  int active_caffe;
  int active_cassa;
} WorkerConfig;

typedef struct group_state {
  // n° di utenti del gruppo [i] arrivati alla barriera
  int arrived_count[MAX_GROUPS];
} GroupState;

///////////////////
//  DATA STATES  //
///////////////////

typedef struct kitchen_state {
  int remaining_primi;
  int remaining_secondi;
  int remaining_caffe;
} KitchenState;

/////////////////////////////////
//  MESSAGGIO (msgsnd/msgrcv)  //
/////////////////////////////////

typedef struct message_request {
  long mtype;
  pid_t sender_pid;

  // [0]=Primo, [1]=Secondo, [2]=Caffè (flag booleani)
  int food_choice[3];

  int wants_ticket;
  double total_cost;
} MessageRequest;

#define REQ_PAYLOAD_SIZE (sizeof(MessageRequest) - sizeof(long))

typedef enum {
  ORDER_SUCCESS = 0, // ordine eseguito
  ORDER_SOLD_OUT = 1 // cibo finito
} OrderStatus;

typedef struct message_response {
  long mtype;
  pid_t operator_pid;

  OrderStatus status;
} MessageResponse;

#define RES_PAYLOAD_SIZE (sizeof(MessageResponse) - sizeof(long))

typedef enum { OP_PRIMI = 0, OP_SECONDI, OP_CAFFE, OP_CASSA } OpType;

#endif