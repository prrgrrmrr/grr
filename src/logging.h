#ifndef LOGGING_H
#define LOGGING_H

#include "types.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define MAX_LOG_MSG_LENGTH 65536

typedef enum GRR_LOG_LEVEL {
  GRR_LOG_LEVEL_CRITICAL,
  GRR_LOG_LEVEL_ERROR,
  GRR_LOG_LEVEL_WARNING,
  GRR_LOG_LEVEL_INFO,
  GRR_LOG_LEVEL_DEBUG
} GRR_LOG_LEVEL;

#define GRR_LOG_CRITICAL(fmtString, ...)                                       \
  Grr_log(GRR_LOG_LEVEL_CRITICAL, fmtString, ##__VA_ARGS__)

#define GRR_LOG_ERROR(fmtString, ...)                                          \
  Grr_log(GRR_LOG_LEVEL_ERROR, fmtString, ##__VA_ARGS__)

#define GRR_LOG_WARNING(fmtString, ...)                                        \
  Grr_log(GRR_LOG_LEVEL_WARNING, fmtString, ##__VA_ARGS__)

#define GRR_LOG_INFO(fmtString, ...)                                           \
  Grr_log(GRR_LOG_LEVEL_INFO, fmtString, ##__VA_ARGS__)

#if defined(GRR_DEBUG)
#define GRR_LOG_DEBUG(fmtString, ...)                                          \
  Grr_log(GRR_LOG_LEVEL_DEBUG, fmtString, ##__VA_ARGS__)
#else
#define GRR_LOG_DEBUG(fmtString, ...)
#endif

void Grr_log(GRR_LOG_LEVEL level, const Grr_string msg, ...);

#endif