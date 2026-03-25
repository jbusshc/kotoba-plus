#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL
} LogLevel;

typedef struct {
    FILE* log_file;
    LogLevel min_level;
    bool console_output;
    pthread_mutex_t mutex;
    bool initialized;
} Logger;

extern Logger logger;

void logger_init(const char* filename, LogLevel level, bool console);
void logger_log(LogLevel level, const char* message);

void logger_debug(const char* message);
void logger_info(const char* message);
void logger_warning(const char* message);
void logger_error(const char* message);
void logger_critical(const char* message);

void logger_set_level(LogLevel level);
void logger_set_console_output(bool enable);
void logger_cleanup(void);

#define LOG_DEBUG(msg) logger_debug(msg)
#define LOG_INFO(msg) logger_info(msg)
#define LOG_WARNING(msg) logger_warning(msg)
#define LOG_ERROR(msg) logger_error(msg)
#define LOG_CRITICAL(msg) logger_critical(msg)

#endif // LOG_H
