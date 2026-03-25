#include "log.h"
#include <stdlib.h>
#include <string.h>

Logger logger = {0};

static const char* log_level_string(LogLevel level) {
    switch (level) {
        case LOG_DEBUG:    return "DEBUG";
        case LOG_INFO:     return "INFO";
        case LOG_WARNING:  return "WARNING";
        case LOG_ERROR:    return "ERROR";
        case LOG_CRITICAL: return "CRITICAL";
        default:           return "UNKNOWN";
    }
}

void logger_init(const char* filename, LogLevel level, bool console) {
    if (logger.initialized) return;
    
    pthread_mutex_init(&logger.mutex, NULL);
    logger.min_level = level;
    logger.console_output = console;
    
    if (filename) {
        logger.log_file = fopen(filename, "a");
    }
    
    logger.initialized = true;
}

void logger_log(LogLevel level, const char* message) {
    if (!logger.initialized || level < logger.min_level || !message) return;
    
    pthread_mutex_lock(&logger.mutex);
    
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    char log_entry[512];
    snprintf(log_entry, sizeof(log_entry), "[%s] %s: %s\n", 
             timestamp, log_level_string(level), message);
    
    if (logger.console_output) {
        fprintf(stdout, "%s", log_entry);
        fflush(stdout);
    }
    
    if (logger.log_file) {
        fprintf(logger.log_file, "%s", log_entry);
        fflush(logger.log_file);
    }
    
    pthread_mutex_unlock(&logger.mutex);
}

void logger_debug(const char* message) {
    logger_log(LOG_DEBUG, message);
}

void logger_info(const char* message) {
    logger_log(LOG_INFO, message);
}

void logger_warning(const char* message) {
    logger_log(LOG_WARNING, message);
}

void logger_error(const char* message) {
    logger_log(LOG_ERROR, message);
}

void logger_critical(const char* message) {
    logger_log(LOG_CRITICAL, message);
}

void logger_set_level(LogLevel level) {
    if (!logger.initialized) return;
    pthread_mutex_lock(&logger.mutex);
    logger.min_level = level;
    pthread_mutex_unlock(&logger.mutex);
}

void logger_set_console_output(bool enable) {
    if (!logger.initialized) return;
    pthread_mutex_lock(&logger.mutex);
    logger.console_output = enable;
    pthread_mutex_unlock(&logger.mutex);
}

void logger_cleanup(void) {
    if (!logger.initialized) return;
    pthread_mutex_lock(&logger.mutex);
    
    if (logger.log_file) {
        fclose(logger.log_file);
        logger.log_file = NULL;
    }
    
    logger.initialized = false;
    pthread_mutex_unlock(&logger.mutex);
    pthread_mutex_destroy(&logger.mutex);
}