#ifndef MENU_H
#define MENU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common/config.h"
#include "common/logger.h"
#include "types.h"

typedef struct dish_list {
  Dish *dishes;
  int count;
  int capacity;
} DishList;

/**
 * @brief Carica il file in una lista interna (chiamato solo dal Responsabile)
 *
 * @param filepath Percorso del file menu.txt
 * @return int 0 se successo, -1 altrimenti
 */
int menu_init(const char *filepath);

/**
 * @brief Genera il menu giornaliero scegliendo piatti casuali
 *
 * Default: 2 primi, 2 secondi, 4 dolci e 5 caffè
 *
 * @param menu Puntatore alla struct DailyMenu da riempire
 */
void generate_daily_menu(DailyMenu *menu, const Config *config);

/**
 * @brief Libera le risorse allocate per il menu
 */
void menu_destroy(void);

#endif