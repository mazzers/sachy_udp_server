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
 * File: receiver.c
 * Description: Receiver thread, handles sending datagrams to clients
 * 
 * -----------------------------------------------------------------------------
 * 
 * @author: Martin Kucera, 2014
 * @version: 1.02
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>

#include "global.h"
#include "server.h"
#include "logger.h"
#include "err.h"
#include "com.h"

/**
 * void *start_receiving(void *arg)
 * 
 * Entry point for receiving thread. Socket timeout for receiving calls
 * is se to 1s so that the receiving thread can check every second if main
 * thread didn' ask him to terminate. All accepted datagrams passes to
 * process_dgram function.
 */
void *start_receiving(void *arg) {
    unsigned int client_len;
    int n;
    struct sockaddr_in client_addr;
    char dgram[MAX_DGRAM_SIZE];
    pthread_mutex_t *thr_mutex = (pthread_mutex_t *) arg;
    
    client_len = sizeof(client_addr);
    
    /* Init rand */
    srand(time(NULL));
    
    /* Ticks each second cehcking if thread is still alive */
    while(!stop_thread(thr_mutex)) {
        n = recvfrom(server_sockfd, &dgram, sizeof(dgram), 0,
                (struct sockaddr *) &client_addr, &client_len);
        
        /* Got data */
        if(n > 0) {            
            process_dgram(dgram, &client_addr);
            
            /* Stats */
            recv_bytes += n;
            recv_dgrams++;
        }
    }
    
    log_line("SERV: Receiving thread terminated.", LOG_ALWAYS);
    
    pthread_exit(NULL);
}
