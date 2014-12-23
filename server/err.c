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
 * File: err.c
 * Description: Provides basic functionality for handling errors.
 * 
 * -----------------------------------------------------------------------------
 * 
 * @author: Martin Kucera, 2014
 * @version: 1.02
 * 
 */

#include <stdio.h>
#include <stdlib.h>

#include "logger.h"

/**
 * void raise_error(char *msg)
 * 
 * Interrupts program execution, prints error message and exits with EXIT_FAILURE
 * code.
 */
void raise_error(char *msg) {
    if(!log_line(msg, LOG_ALWAYS)) {
        fprintf(stderr, "Error: %s\n", msg);
    }
    
    exit(EXIT_FAILURE);
}
