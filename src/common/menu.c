#include "common/menu.h"

typedef struct {
  Dish *dishes;
  int count;
  int capacity;
} DishList;

static DishList all_primi = {0};
static DishList all_secondi = {0};
static DishList all_dolci = {0};
static DishList all_caffe = {0};

static void add_dish(char type, const char *name) {
  DishList *list;
  switch (type) {
  case 'P':
    list = &all_primi;
    break;
  case 'S':
    list = &all_secondi;
    break;
  case 'D':
    list = &all_dolci;
    break;
  case 'C':
    list = &all_caffe;
    break;
  default:
    return;
  }

  if (list->count == list->capacity) {
    list->capacity = (list->capacity == 0) ? 10 : list->capacity * 2;
    list->dishes = realloc(list->dishes, sizeof(Dish) * (long unsigned int)list->capacity);
  }

  list->dishes[list->count].type = type;
  strncpy(list->dishes[list->count].name, name, MAX_DISH_NAME - 1);
  list->dishes[list->count].name[MAX_DISH_NAME - 1] = '\0';
  list->count++;
}

int menu_init(const char *filepath) {
  FILE *f = fopen(filepath, "r");
  if (!f) {
    return -1;
  }

  char line[128];
  while (fgets(line, sizeof(line), f)) {
    // formato: T|Nome
    char *separator = strchr(line, '|');
    if (separator) {
      *separator = '\0';
      char type = line[0];
      char *name = separator + 1;

      name[strcspn(name, "\n")] = 0;
      add_dish(type, name);
    }
  }

  fclose(f);
  return 0;
}

static void pick_random_dishes(DishList *src, Dish *dest, int max_needed,
                               int *out_count) {
  if (src->count == 0) {
    *out_count = 0;
    return;
  }

  int needed = (src->count < max_needed) ? src->count : max_needed;
  *out_count = needed;

  int *indices = malloc(sizeof(int) * (long unsigned int)src->count);
  if (!indices) {
    LOG_ERR("MENU", "Malloc failed in pick_random_dishes");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < src->count; i++) {
    indices[i] = i;
  }

  for (int i = 0; i < needed; i++) {
    int j = i + rand() % (src->count - i);

    int temp = indices[i];
    indices[i] = indices[j];
    indices[j] = temp;

    dest[i] = src->dishes[indices[i]];
  }

  free(indices);
}

void generate_daily_menu(DailyMenu *menu, const Config *config) {
  memset(menu, 0, sizeof(DailyMenu));

  // 2 primi, 2 secondi, 4 dolci e 5 caffè
  pick_random_dishes(&all_primi, menu->daily_primi, config->daily_primi_count, &menu->primi_count);
  pick_random_dishes(&all_secondi, menu->daily_secondi, config->daily_secondi_count,
                     &menu->secondi_count);
  pick_random_dishes(&all_dolci, menu->daily_dolci, config->daily_dolci_count,
                     &menu->dolci_count);
  pick_random_dishes(&all_caffe, menu->daily_caffe, config->daily_caffe_count, &menu->caffe_count);
}

void menu_destroy(void) {
  free(all_primi.dishes);
  all_primi.dishes = NULL; all_primi.count = 0; all_primi.capacity = 0;

  free(all_secondi.dishes);
  all_secondi.dishes = NULL; all_secondi.count = 0; all_secondi.capacity = 0;

  free(all_dolci.dishes);
  all_dolci.dishes = NULL; all_dolci.count = 0; all_dolci.capacity = 0; 

  free(all_caffe.dishes);
  all_caffe.dishes = NULL; all_caffe.count = 0; all_caffe.capacity = 0;
}