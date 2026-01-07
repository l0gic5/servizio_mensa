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

// semafori (risorse)
#define SEM_INDEX_TABLES 4
#define SEM_INDEX_MUTEX_STATS 5
#define SEM_INDEX_OUTPUT 6
#define SEM_INDEX_BARRIER 7

// semafori (operatori)
#define SEM_OPERATORS_PRIMI 8
#define SEM_OPERATORS_SECONDI 9
#define SEM_OPERATORS_CAFFE 10
#define SEM_OPERATORS_CASSA 11

#define TOTAL_SEMS 12
#define MAX_WORKERS 100

typedef struct worker_config {
  // indice worker -> enum OpType
  int worker_roles[MAX_WORKERS];
  int total_workers_count;

  volatile time_t strike_end_times[MAX_WORKERS];

  int active_primi;
  int active_secondi;
  int active_caffe;
  int active_cassa;
} WorkerConfig;

///////////////////
//  STATISTICHE  //
///////////////////

typedef struct kitchen_state {
  int remaining_primi;
  int remaining_secondi;
  int remaining_caffe;
} KitchenState;

typedef struct global_stats {
  int total_users_served;
  int total_users_refused;

  // piatti distribuiti
  int total_plates_primi;
  int total_plates_secondi;
  int total_plates_caffe;

  // accumulatori tempo di attesa
  double total_wait_time_primi;
  double total_wait_time_secondi;
  double total_wait_time_caffe;
  double total_wait_time_cassa;

  double total_revenue;
  int total_transactions;
} GlobalStats;

typedef struct daily_report {
  int day_number;

  // delta (oggi - ieri)
  int daily_users_served;
  int daily_users_refused;

  int daily_plates_primi;
  int daily_plates_secondi;
  int daily_plates_caffe;

  // delta tempi attesa
  double daily_wait_primi;
  double daily_wait_secondi;
  double daily_wait_caffe;
  double daily_wait_cassa;

  double daily_revenue;
  int daily_transactions;

  // avanzi
  int leftover_primi;
  int leftover_secondi;
  int leftover_caffe;
} DailyReport;

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