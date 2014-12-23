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

#ifndef LOGGER_H
#define	LOGGER_H

/* Log levels */
#define LOG_ALWAYS -1
#define LOG_NONE 0
#define LOG_ERR 1
#define LOG_WARN 2
#define LOG_INFO 3
#define LOG_DEBUG 4
#define LOG_ALL 5

#define DEFAULT_LOGFILE ups_servlog.log

#define LOG_BUFFER_SIZE 1024

extern int log_level;
extern int verbose_level;

/* Function prototypes */
void init_logger(char *filename);
int log_line(char *msg, int severity);
void stop_logger();

#endif	/* LOGGER_H */

