#include "logger.h"

LogLevel log_level = LOG_LEVEL;

const char* get_log_string(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return " DEBUG ";
        case LOG_INFO: return " INFO  ";
        case LOG_WARN: return "WARNING";
        case LOG_ERROR: return " ERROR ";
        default: return "UNKNOWN";
    }
}

void set_log_level(LogLevel ll) {
    log_info("Changing LogLevel: %s(%d) -> %s(%d)",get_log_string(log_level) ,log_level, get_log_string(ll), ll);
    log_level = ll;
}

int get_log_level(){
    return log_level;
}

const char *get_log_level_str(){
    return get_log_string(log_level);
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
    if (level > log_level) return;
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
