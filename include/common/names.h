#ifndef NAMES_H
#define NAMES_H

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <time.h>
#include <unistd.h>

#include "common/logger.h"

#define PATH_NOMI_M "data/nomi_maschili.txt"
#define PATH_NOMI_F "data/nomi_femminili.txt"
#define PATH_COGNOMI "data/cognomi.txt"

typedef enum { ROLE_RESPONSABILE, ROLE_OPERATORE, ROLE_CASSA, ROLE_UTENTE } PersonRole;

/**
 * @brief Carica i file di testo in memoria.
 * Deve essere chiamata una volta all'inizio del main di ogni processo.
 *
 * * @return 0 su successo, -1 se un file non viene trovato.
 */
int names_init(void);

/**
 * @brief Libera la memoria allocata per le liste di nomi.
 */
void names_destroy(void);

/**
 * @brief Genera una stringa identificativa formattata.
 * Esempio: "[U] Mario Rossi (1234)" o "[O] Luigi V. (5678)"
 *
 * * @param role Il ruolo per determinare il prefisso e il formato
 * @param pid Il PID del processo
 *
 * @return char* Stringa allocata dinamicamente (da liberare con free)
 */
char *get_random_identity(PersonRole role);

#endif