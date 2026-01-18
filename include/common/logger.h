#ifndef LOGGER_H
#define LOGGER_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_PURPLE "\x1b[35m"
#define COLOR_PINK "\x1b[95m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_ORANGE "\x1b[38;5;208m"
#define COLOR_DARK_GRAY "\x1b[90m"
#define COLOR_LIGHT_GRAY "\x1b[37m"
#define COLOR_RESET "\x1b[0m"

#define LOG_CTX_WIDTH 38

#define FLUSH_LOGS                                                             \
  do {                                                                         \
    fflush(stdout);                                                            \
    fflush(stderr);                                                            \
  } while (0)

#define TEST_ERROR                                                             \
  if (errno) {                                                                 \
    fprintf(stderr,                                                            \
            COLOR_RED "%s:%d: PID=%5d: Errore %d (%s)" COLOR_RESET "\n",       \
            __FILE__, __LINE__, getpid(), errno, strerror(errno));             \
  }

#define LOG_DEBUG(ctx, msg, ...)                                               \
  fprintf(stdout,                                                              \
          COLOR_ORANGE "%s" COLOR_PURPLE "(%d) " COLOR_ORANGE                  \
                       "DEBUG: " msg COLOR_RESET "\n",                         \
          ctx, getpid(), ##__VA_ARGS__)

#define EXIT_ON_ERROR                                                          \
  if (errno) {                                                                 \
    TEST_ERROR;                                                                \
    exit(EXIT_FAILURE);                                                        \
  }

#define ROLE_NAME(r)                                                           \
  ((r) == 0   ? "PRIMI"                                                        \
   : (r) == 1 ? "SECONDI"                                                      \
   : (r) == 2 ? "DOLCI"                                                        \
   : (r) == 3 ? "CAFFE"                                                        \
   : (r) == 4 ? "CASSA"                                                        \
              : "IGNOTO")

#define ROLE_NAME_SINGULAR(r)                                                  \
  ((r) == 0   ? "PRIMO"                                                        \
   : (r) == 1 ? "SECONDO"                                                      \
   : (r) == 2 ? "DOLCE"                                                        \
   : (r) == 3 ? "CAFFE"                                                        \
   : (r) == 4 ? "CASSA"                                                        \
              : "IGNOTO")

#define LOG_INFO(ctx, msg, ...)                                                \
  fprintf(stdout, COLOR_GREEN "%s " COLOR_PURPLE "(%d) " COLOR_RESET msg "\n", \
          ctx, getpid(), ##__VA_ARGS__)

#define LOG_CONF(ctx, msg, ...)                                                \
  do {                                                                         \
    fprintf(stdout,                                                            \
            COLOR_PINK "%s " COLOR_PURPLE "(%d) " COLOR_PINK                   \
                       "CONF: " msg COLOR_RESET "\n",                          \
            ctx, getpid(), ##__VA_ARGS__);                                     \
    fflush(stdout);                                                            \
  } while (0)

#define LOG_WARN(ctx, msg, ...)                                                \
  fprintf(stdout,                                                              \
          COLOR_YELLOW "%s " COLOR_PURPLE "(%d) " COLOR_YELLOW                 \
                       "WARNING: " msg COLOR_RESET "\n",                       \
          ctx, getpid(), ##__VA_ARGS__)

#define LOG_ERR(ctx, msg, ...)                                                 \
  fprintf(stderr,                                                              \
          COLOR_RED "%s " COLOR_PURPLE "(%d) " COLOR_RED                       \
                    "ERROR: " msg COLOR_RESET "\n",                            \
          ctx, getpid(), ##__VA_ARGS__)

#define SIMULATION_HEADER                                                      \
  "       .   ~   ~   ~   ~   ~   ~   .       \n"                              \
  "   ~  .  -------------------------  .  ~   \n"                              \
  " ~  (        Oasi del Golfo       )  ~ \n"                                  \
  "~    )         Da Marina         (    ~\n"                                  \
  "   ~  '  -------------------------  '  ~   \n"                              \
  "       ^   ~   ~   ~   ~   ~   ~   ^       \n"

/**
 * @brief Calcola l'offset tra byte e caratteri visibili in una stringa UTF-8.
 *
 * Utile per allineamenti di output con caratteri speciali (es: accenti).
 *
 * @param s Stringa UTF-8 da analizzare.
 * @return Numero di byte in più rispetto ai caratteri visibili.
 */
static inline int get_utf8_offset(const char *s) {
  int len_bytes = 0;
  int len_chars = 0;

  while (*s) {
    if ((*s & 0xC0) != 0x80) {
      len_chars++;
    }
    len_bytes++;
    s++;
  }

  return len_bytes - len_chars;
}

/**
 * @brief Restituisce una stringa di spazi per l'indentazione nei log.
 *
 * @param n Numero di spazi desiderati.
 * @return const char* Puntatore a una stringa contenente n spazi.
 */
static inline const char *log_spaces(int n) {
  // 256
  static const char padding[] =
      "                                                                "
      "                                                                ";

  const int max_len = sizeof(padding) - 1;

  if (n < 0) {
    n = 0;
  }
  if (n > max_len) {
    n = max_len;
  }

  return padding + (max_len - n);
}

#endif