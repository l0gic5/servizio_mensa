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
#define SEM_INDEX_SEATS_COFFEE 2
#define SEM_INDEX_SEATS_CASSA 3
// posti a sedere "NOF_TABLE_SEATS"
#define SEM_INDEX_TABLES 4
// protezione scrittura statistiche
#define SEM_INDEX_MUTEX_STATS 5
// evita printf accavallate
#define SEM_INDEX_OUTPUT 6

#define TOTAL_SEMS 7

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
  int plates_coffee;

  // piatti avanzati (calcolati a fine giornata)
  int leftover_primi;
  int leftover_secondi;
  int leftover_coffee;

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

#endif