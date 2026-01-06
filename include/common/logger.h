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
#define COLOR_CYAN "\x1b[36m"
#define COLOR_RESET "\x1b[0m"

#define TEST_ERROR                                                             \
  if (errno) {                                                                 \
    fprintf(stderr,                                                            \
            COLOR_RED "%s:%d: PID=%5d: Errore %d (%s)" COLOR_RESET "\n",       \
            __FILE__, __LINE__, getpid(), errno, strerror(errno));             \
  }

#define EXIT_ON_ERROR                                                          \
  if (errno) {                                                                 \
    TEST_ERROR;                                                                \
    exit(EXIT_FAILURE);                                                        \
  }

#define ROLE_NAME(r)                                                           \
  ((r) == 0   ? "PRIMI"                                                        \
   : (r) == 1 ? "SECONDI"                                                      \
   : (r) == 2 ? "COFFEE"                                                       \
   : (r) == 3 ? "CASSA"                                                        \
              : "IGNOTO")

#define ROLE_NAME_SINGULAR(r)                                                  \
  ((r) == 0   ? "PRIMO"                                                        \
   : (r) == 1 ? "SECONDO"                                                      \
   : (r) == 2 ? "CAFFE"                                                        \
   : (r) == 3 ? "CASSA"                                                        \
              : "IGNOTO")

#define LOG_INFO(ctx, msg, ...)                                                \
  fprintf(stdout,                                                              \
          COLOR_GREEN "[%s]" COLOR_PURPLE "(%d) " COLOR_RESET msg "\n", ctx,   \
          getpid(), ##__VA_ARGS__)

#define LOG_WARN(ctx, msg, ...)                                                \
  fprintf(stdout,                                                              \
          COLOR_YELLOW "[%s]" COLOR_PURPLE "(%d) " COLOR_YELLOW                \
                       "WARNING: " msg COLOR_RESET "\n",                       \
          ctx, getpid(), ##__VA_ARGS__)

#define LOG_ERR(ctx, msg, ...)                                                 \
  fprintf(stderr,                                                              \
          COLOR_RED "[%s]" COLOR_PURPLE "(%d) " COLOR_RED                      \
                    "ERROR: " msg COLOR_RESET "\n",                            \
          ctx, getpid(), ##__VA_ARGS__)

#endif