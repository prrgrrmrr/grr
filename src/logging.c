#include "logging.h"

void Grr_log(GRR_LOG_LEVEL level, const Grr_string msg, ...) {
  static char fmtString[MAX_LOG_MSG_LENGTH + 1];
  static const Grr_string levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO",
                                      "DEBUG"};
  static const Grr_string ansiColorCodes[] = {
      "\x1b[41m", // Background Red
      "\x1b[31m", // Red
      "\x1b[33m", // Yellow
      "\x1b[32m", // Green
      "\x1b[36m", // Cyan
  };
  static const Grr_string colorReset = "\x1b[0m";       // Reset
  static const Grr_string bgdReset = "\x1b[0m\x1b[31m"; // For critical msgs

  va_list args;
  va_start(args, msg);
  snprintf(fmtString, MAX_LOG_MSG_LENGTH + 1, "%s[%s]%s %s%s",
           ansiColorCodes[level], levels[level],
           (level == GRR_LOG_LEVEL_CRITICAL ? bgdReset : ""), msg, colorReset);
  vprintf(fmtString, args);
  va_end(args);
}