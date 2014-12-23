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
 * File: global.c
 * Description: Provides generic functions.
 * 
 * -----------------------------------------------------------------------------
 * 
 * @author: Martin Kucera, 2014
 * @version: 1.02
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h> 
#include <sys/socket.h>
#include <errno.h> 
#include <netdb.h> 
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>

#include "logger.h"
#include "server.h"

/* Logger buffer */
char log_buffer[LOG_BUFFER_SIZE];

/**
 * void gen_random(char *s, const int len)
 * 
 * Generates random string of length len with all upercase letters and saves it 
 * into the s pointer. String is terminated by null character
 */
void gen_random(char *s, const int len) {
    int i;
    static const char alphanum[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    ;

    for (i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

/**
 * int stop_thread(pthread_mutex_t *mtx)
 * 
 * Checks if thread should stop (mutex is unlocked)
 */
int stop_thread(pthread_mutex_t *mtx) {
    switch(pthread_mutex_trylock(mtx)) {
        case 0:
            pthread_mutex_unlock(mtx);
            return 1;
        case EBUSY:
            return 0;
    }

    return 1;
}

/**
 * int rand_lim(int limit)
 * 
 * Generates number from 1 to limit
 */
int rand_lim(int limit) {
    int divisor = RAND_MAX/(limit+1);
    int retval;

    do { 
        retval = rand() / divisor;
    } while (retval > limit || retval == 0);

    return retval;
}

/**
 * int hostname_to_ip(char *hostname, char *ip)
 * 
 * Attempts to resolve given hostname. If resolved correctly, returns
 * string representation of hostname's ip address in ip and returns 1, otherwise
 * returns 0
 */
int hostname_to_ip(char *hostname, char *ip) {
    struct addrinfo hints, *res;
    struct sockaddr_in *res_addr;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;
    
    if( (getaddrinfo(hostname, NULL, &hints, &res) == 0) ) {
        if(res) {
            res_addr = (struct sockaddr_in *) res->ai_addr;
            strcpy(ip, inet_ntoa(res_addr->sin_addr));
            
            return 1;
        }
    }
    
    return 0;
}

/**
 * void display_uptime()
 * 
 * Shows amount of time elapsed from server start
 */
void display_uptime() {
    struct timeval temp_tv;
    
    gettimeofday(&temp_tv, NULL);

    /* Elapsed time */
    sprintf(log_buffer,
            "Server uptime: %01.0fh:%02.0fm:%02.0fs",
            floor( (temp_tv.tv_sec - ts_start.tv_sec) / 3600.),
            floor(fmod((temp_tv.tv_sec - ts_start.tv_sec), 3600.0) / 60.0),
            fmod((temp_tv.tv_sec - ts_start.tv_sec), 60.0)
            );
    log_line(log_buffer, LOG_ALWAYS);
}
