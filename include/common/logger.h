#ifndef LOGGER_H
#define LOGGER_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define RESET "\x1b[0m"

#define TEST_ERROR                                                             \
  if (errno) {                                                                 \
    fprintf(stderr, RED "%s:%d: PID=%5d: Errore %d (%s)" RESET "\n", __FILE__, \
            __LINE__, getpid(), errno, strerror(errno));                       \
  }

#define EXIT_ON_ERROR                                                          \
  if (errno) {                                                                 \
    TEST_ERROR;                                                                \
    exit(EXIT_FAILURE);                                                        \
  }

#define LOG_INFO(ctx, msg, ...)                                                \
  fprintf(stdout, GREEN "[%s][%d] " msg RESET "\n", ctx, getpid(),             \
          ##__VA_ARGS__)

#define LOG_WARN(ctx, msg, ...)                                                \
  fprintf(stdout, YELLOW "[%s][%d] WARNING: " msg RESET "\n", ctx, getpid(),   \
          ##__VA_ARGS__)

#define LOG_ERR(ctx, msg, ...)                                                 \
  fprintf(stderr, RED "[%s][%d] ERROR: " msg RESET "\n", ctx, getpid(),        \
          ##__VA_ARGS__)

#endif