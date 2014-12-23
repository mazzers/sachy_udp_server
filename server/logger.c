/** 
 * -----------------------------------------------------------------------------
 * Clovece nezlob se (Server) - simple board game
 * 
 * Server for board game Clovece nezlob se using UDP datagrams for communication
 * with clients and SEND-AND-WAIT method to ensure that all packets arrive
 * and that they arrive in correct order. 
 * 
 * Semestral work for "Uvod do pocitacovich siti" KIV/UPS at
 * University of West Bohemia.
 * 
 * -----------------------------------------------------------------------------
 * 
 * File: logger.c
 * Description: Handles all logging.
 * 
 * -----------------------------------------------------------------------------
 * 
 * @author: Martin Kucera, 2014
 * @version: 1.02
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#include "global.h"
#include "err.h"
#include "logger.h"

/* Current log level, can be changed during runtime */
int log_level = LOG_WARN;
/* Current verbose level, can be changed during runtime  */
int verbose_level = LOG_INFO;

/* Output logfile */
FILE *logfile = NULL;

/* Timer used for timestamp */
time_t timer;
/* Tm info */
struct tm *tm_info;

/**
 * void init_logger(char *filename)
 * 
 * Opens file with filename in append mode and writes info about new logging
 * session.
 */
void init_logger(char *filename) {
    char *buff;
    
    if((logfile = fopen(filename, "a+")) == NULL) {
        raise_error("Error opening logging file\n");
    }
    
    buff = (char *) malloc(strlen(filename) + 19);
    
    sprintf(buff,
            "Logging to file: %s",
            filename
            );
    
    log_line(buff, LOG_ALWAYS);
    
    free(buff);
    fprintf(logfile, "\n--------------------------------------------------------\n");
    log_line("Logger started", LOG_ALWAYS);
}

/**
 * int log_line(char *msg, int severity)
 * 
 * Logs given message. If severity is below or equal to log_level, message
 * is written to log file. If severity is below or equal to verbose_level, message
 * is written to console. Both can happen
 */
int log_line(char *msg, int severity) {
    char timestamp[25];
    char *buff;
    
    if(logfile && (severity <= log_level || severity <= verbose_level)) {
        time(&timer);
        tm_info = localtime(&timer);

        strftime(timestamp, 25, "%d.%m.%y, %H:%M:%S: ", tm_info);

        buff = (char *) malloc(strlen(msg) + 27);

        sprintf(buff,
                "%s%s\n",
                timestamp,
                msg
                );

        /* Write to log file */
        if(severity <= log_level) {
            fputs(buff, logfile);
	    fflush(logfile);
        }
        
        /* Write to console */
        if(severity <= verbose_level) {
            fputs(buff, stdout);
        }
        
        free(buff);
        
        return 1;
    }
    
    return 0;
}

/**
 * void stop_logger()
 * 
 * Logs message indicating that logger is stopping and closes logfile
 */
void stop_logger() {
    log_line("Stopping logger", LOG_ALWAYS);
    fclose(logfile);
}
