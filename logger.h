#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#ifdef _WIN32
#define __SHORT_FILE__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define LOG_PATH "log.log"
#ifndef LOG_LEVEL
    #define LOG_LEVEL 4
#endif
#define LOG_OPTIONS_COUNT 4

typedef enum {
    LOG_DEBUG = 4,
    LOG_INFO = 3,
    LOG_WARN = 2,
    LOG_ERROR = 1
} LogLevel;

/* logs the msg with error lvl. */
#define log_error(msg, ...) log_message(LOG_ERROR, __SHORT_FILE__, __func__, __LINE__, msg, ## __VA_ARGS__)
/* logs the msg with warn lvl. */
#define log_warn(msg, ...) log_message(LOG_WARN, __SHORT_FILE__, __func__, __LINE__, msg, ## __VA_ARGS__)
/* logs the msg with info lvl. */
#define log_info(msg, ...) log_message(LOG_INFO, __SHORT_FILE__, __func__, __LINE__, msg, ## __VA_ARGS__)
/* logs the msg with debug lvl. */
#define log_debug(msg, ...) log_message(LOG_DEBUG, __SHORT_FILE__, __func__, __LINE__, msg, ## __VA_ARGS__)
/* logs the msg with the given log LogLevel level. */
#define log_with_level(level, msg, ...) log_message(level, __SHORT_FILE__, __func__, __LINE__, msg, ## __VA_ARGS__)

/*returns the string used for a LogLevel.*/
void set_log_level(LogLevel level);
int get_log_level_int();
const char *get_log_level_str();
const char *get_log_string(LogLevel level);

void log_message_string(LogLevel level, char *file, const char* func, const int line, char* msg);
void log_message(LogLevel level, char* file, const char *func, const int line, char* msg, ...);


#endif /* LOGGER_H */
