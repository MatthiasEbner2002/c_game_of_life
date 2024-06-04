#include "logger.h"

const char* get_log_string(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return " DEBUG ";
        case LOG_INFO: return " INFO  ";
        case LOG_WARN: return "WARNING";
        case LOG_ERROR: return " ERROR ";
        default: return "UNKNOWN";
    }
}

void get_timestamp(char *buffer) {
    struct timeval tv;
    time_t nowtime;
    struct tm *nowtm;
    char tmbuf[64];

    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec;
    nowtm = localtime(&nowtime);
    strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
    snprintf(buffer, 80, "%s:%06ld", tmbuf, tv.tv_usec);
}


void log_message_string(LogLevel level, char *file, const char* func, const int line, char* msg){
    if (level > LOG_LEVEL) return;
    char timestamp[80];  // the timestamp of the log message, max size is 80
    get_timestamp(timestamp);

    FILE* log_file = fopen(LOG_PATH, "a");
    if (log_file == NULL) {
        fprintf(stderr, "Error opening log file\n");
        return;
    }
    fprintf(log_file, "[%s] [%s] [%s - %s(): %d]: %s\n", timestamp, get_log_string(level), file, func, line, msg);
    fclose(log_file);
}

void log_message(LogLevel level, char* file, const char *func, const int line, char* msg, ...){
    int buffersize = 2024;  // the max size of the log message
    char *buffer = malloc(buffersize * sizeof(char));
    va_list args;
    va_start(args, msg);
    vsnprintf(buffer, buffersize, msg, args);
    log_message_string(level, file, func, line, buffer);
    va_end(args);
    free(buffer);
}
