

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#include "global.h"
#include "logger.h"

/*Default log and verbose levels*/
int log_level = LOG_WARN;
int verbose_level = LOG_INFO;
/*Output file*/
FILE *logfile = NULL;
/*Timers*/
time_t timer;
struct tm *tm_info;

/*
___________________________________________________________

    Logger initiation. Create logfile.
___________________________________________________________
*/
 void init_logger(char *filename) {
    char *buff;
    
    if((logfile = fopen(filename, "a+")) == NULL) {
        raise_error("Error opening logging file");
        
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

/*
___________________________________________________________

    Write line to log file. 
___________________________________________________________
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

        if(severity <= log_level) {
            fputs(buff, logfile);
            fflush(logfile);
        }
        
        if(severity <= verbose_level) {
            fputs(buff, stdout);
        }
        
        free(buff);
        
        return 1;
    }
    
    return 0;
}

/*
___________________________________________________________

    Stop server and notify about error.
___________________________________________________________
*/
void raise_error(char *msg) {
    if(!log_line(msg, LOG_ALWAYS)) {
        fprintf(stderr, "Error: %s\n", msg);
    }
    
    exit(EXIT_FAILURE);
}
/*
___________________________________________________________

    Stop logger and close logfile.
___________________________________________________________
*/
 void stop_logger() {
    log_line("Stopping logger", LOG_ALWAYS);
    fclose(logfile);
}
