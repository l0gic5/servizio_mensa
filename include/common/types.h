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

// semafori (operatori)
#define SEM_OPERATORS_PRIMI 7
#define SEM_OPERATORS_SECONDI 8
#define SEM_OPERATORS_CAFFE 9
#define SEM_OPERATORS_CASSA 10

#define TOTAL_SEMS 11
#define MAX_WORKERS 100

typedef struct {
  // indice worker -> enum OpType
  int worker_roles[MAX_WORKERS];
} WorkerConfig;

///////////////////
//  STATISTICHE  //
///////////////////

typedef struct kitchen_state {
  int remaining_primi;
  int remaining_secondi;
  int remaining_caffe;
} KitchenState;

typedef struct {
  int total_users_served;
  int total_users_refused;

  // piatti distribuiti
  int total_plates_primi;
  int total_plates_secondi;
  int total_plates_caffe;

  double total_revenue;
} GlobalStats;

typedef struct {
  int day_number;

  // delta (oggi - ieri)
  int daily_users_served;
  int daily_users_refused;

  int daily_plates_primi;
  int daily_plates_secondi;
  int daily_plates_caffe;

  double daily_revenue;

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