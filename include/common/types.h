#ifndef TYPES_H
#define TYPES_H

#include <sys/types.h>

#define MAX_BUFFER_SIZE 256

// richiesta ordine (opzionale per code separate)
#define MSG_TYPE_ORDER 1

// utente paga alla cassa
#define MSG_TYPE_PAYMENT 2

#define SEM_INDEX_SEATS_PRIMI 0
#define SEM_INDEX_SEATS_SECONDI 1
#define SEM_INDEX_SEATS_CAFFE 2
#define SEM_INDEX_SEATS_CASSA 3
// posti a sedere "NOF_TABLE_SEATS"
#define SEM_INDEX_TABLES 4
// protezione scrittura statistiche
#define SEM_INDEX_MUTEX_STATS 5
// evita printf accavallate
#define SEM_INDEX_OUTPUT 6

#define SEM_OPERATORS_PRIMI 7
#define SEM_OPERATORS_SECONDI 8
#define SEM_OPERATORS_CAFFE 9
#define SEM_OPERATORS_CASSA 10

#define TOTAL_SEMS 11

#define MAX_WORKERS 100

#define MSG_TYPE_ORDER_PRIMI 1
#define MSG_TYPE_ORDER_SECONDI 2
#define MSG_TYPE_ORDER_CAFFE 3

typedef struct {
  // indice worker -> enum OpType
  int worker_roles[MAX_WORKERS];
} WorkerConfig;

///////////////////
//  STATISTICHE  //
///////////////////

typedef struct statistics {
  int total_users_served;
  // utenti non serviti o che rinunciano
  int total_users_refused;

  // piatti distribuiti per tipo
  int plates_primi;
  int plates_secondi;
  int plates_caffe;

  // piatti avanzati (calcolati a fine giornata)
  int leftover_primi;
  int leftover_secondi;
  int leftover_caffe;

  double total_revenue;

  long total_waiting_time;
} Statistics;

/////////////////////////////////
//  MESSAGGIO (msgsnd/msgrcv)  //
/////////////////////////////////

typedef struct message_request {
  long mtype;
  pid_t sender_pid;
  // [0]=Primo, [1]=Secondo, [2]=Caffè (1=preso, 0=no)
  int food_choice[3];
  int wants_ticket;
  double total_cost;
} MessageRequest;

typedef struct message_response {
  // PID destinatario
  long mtype;
  pid_t operator_pid;
} MessageResponse;

typedef enum { OP_PRIMI = 0, OP_SECONDI, OP_CAFFE, OP_CASSA } OpType;

#endif