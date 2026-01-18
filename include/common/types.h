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
#define SEM_INDEX_SEATS_DOLCI 2
#define SEM_INDEX_SEATS_CAFFE 3
#define SEM_INDEX_SEATS_CASSA 4
#define SEM_INDEX_TICKET_READER 5

// semafori (risorse)
#define SEM_INDEX_TABLES 6
#define SEM_INDEX_MUTEX_STATS 7
#define SEM_INDEX_OUTPUT 8
#define SEM_INDEX_BARRIER 9

// semafori (operatori)
#define SEM_OPERATORS_PRIMI 10
#define SEM_OPERATORS_SECONDI 11
#define SEM_OPERATORS_CAFFE 12
#define SEM_OPERATORS_CASSA 13

#define SEM_INDEX_DAY_CHANGE 14

#define MAX_GROUPS 100
#define MAX_TOTAL_USERS 500
#define SEM_GROUP_BARRIER_BASE 15

#define TOTAL_SEMS (15 + MAX_GROUPS)
#define MAX_WORKERS 100

#define MAX_DISH_NAME 32
#define MAX_DAILY_OFFER 30

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

  int user_to_group_map[MAX_TOTAL_USERS];
  
  int group_sizes[MAX_GROUPS];
} GroupState;

///////////////////
//  DATA STATES  //
///////////////////

typedef struct dish {
  char name[MAX_DISH_NAME];
  char type; // 'P', 'S', 'D', 'C'
} Dish;

typedef struct daily_menu {
  Dish daily_primi[MAX_DAILY_OFFER];
  Dish daily_secondi[MAX_DAILY_OFFER];
  Dish daily_dolci[MAX_DAILY_OFFER];
  Dish daily_caffe[MAX_DAILY_OFFER];
  
  int primi_count;
  int secondi_count;
  int dolci_count;
  int caffe_count;
} DailyMenu;

typedef struct kitchen_state {
  int remaining_primi;
  int remaining_secondi;
  int remaining_dolci;
  int remaining_caffe;

  DailyMenu todays_menu;
} KitchenState;

/////////////////////////////////
//  MESSAGGIO (msgsnd/msgrcv)  //
/////////////////////////////////

// [0]=Primo, [1]=Secondo, [2]=Dolce, [3]=Caffè
#define MSG_REQ_PRIMO_INDEX 0
#define MSG_REQ_SECONDO_INDEX 1
#define MSG_REQ_DOLCE_INDEX 2
#define MSG_REQ_CAFFE_INDEX 3

typedef struct message_request {
  long mtype;
  pid_t sender_pid;
  
  // [0]=Primo, [1]=Secondo, [2]=Dolce, [3]=Caffè (flag booleani)
  int food_choice[4];

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

// OP_DOLCI non usato come ruolo operatore
typedef enum { OP_PRIMI = 0, OP_SECONDI, OP_DOLCI, OP_CAFFE, OP_CASSA } OpType;

#endif