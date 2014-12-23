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
 * File: sender.c
 * Description: Handles sending datagram to clients
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
#include <netinet/in.h>

#include "global.h"
#include "client.h"
#include "com.h"
#include "game.h"
#include "logger.h"

/* Condition signaling change in packet status for any client */
pthread_cond_t cond_packet_change;
/* Mutex protection condition variable */
pthread_mutex_t mtx_cond_packet_change;

/**
 * void *start_sending(void *arg)
 * 
 * Entry point for sender thread. Loops through all connected users and checks
 * if they have any packets queued that need to be (re)sent. Also check's their
 * timestamps. After checking all clients, goes to sleep for minimum amount of time
 * before any packet timeouts or maximum number of 500ms, in order to check
 * if the main thread didnt ask him to terminate.
 */
void *start_sending(void *arg) {
    /* Sender mutex */
    pthread_mutex_t *thr_mutex = (pthread_mutex_t *) arg;
    /* Lowest time from all clients before their packet timeouts */
    int wait;
    /* Client index */
    int i;
    /* Temp client */
    client_t *client;
    /* Temp packet from queue_front */
    packet_t *packet;
    /* Timespec for timedwait */
    struct timespec ts;
    /* How many clients we got in a loop */
    unsigned int got_clients;
    
    while(!stop_thread(thr_mutex)) {
        got_clients = 0;
        wait = -1;
        
        for(i = 0; i < MAX_CURRENT_CLIENTS && got_clients <= client_num; i++) {
            /* Get client (lock) */
            client = get_client_by_index(i);
            
            if(client) {
                got_clients++;
               
                /* If client is active */
                if(client->state) {
                    /* Check if client's timestamp is too old */
                    if(client_timestamp_timeout(client)) {
                        
                        /* Attempt to timeout player in current game */
                        raise_error("timeout_game");
                        //timeout_game(client);
                        client->state = 0;
                        
                    } 
                    else if(queue_size(client->dgram_queue) > 0) {
                        packet = queue_front(client->dgram_queue);

                        /* Send new packet or resend packet which is timeouted */
                        while( packet &&  ( ( packet->state == 0) || 
                                ( packet->state == 1 && packet_timestamp_old(*packet, &wait)))) {
                            
                            send_packet(packet, client);
                            
                            if(!packet->req_ack) {
                                queue_pop(client->dgram_queue, 0);
                                
                                free(packet->msg);
                                free(packet->payload);
                                free(packet);
                                
                                packet = queue_front(client->dgram_queue);
                            }
                            else {
                                packet = NULL;
                            }
                            
                        }
                    }
                }
                else if(client_timestamp_remove(client)){
                    if(client->game_index != -1) {
                        raise_error("leave_game");
                        //leave_game(client);
                    }
                    raise_error("remove_client");
                    //remove_client(&client);

                }
                
                if(client) {
                    release_client(client);
                }
            }
        }
        
        /* If no packets are waiting, set wait to default value, periodically
         * checking if thread is still alive
         */
        if(wait < 0 || wait > 500000) {
            wait = 500000;
        }
        
        pthread_mutex_lock(&mtx_cond_packet_change);
        clock_gettime(CLOCK_REALTIME, &ts);
        
        if((ts.tv_nsec + (wait * 1000)) > NANOSECONDS_IN_SECOND) {
            ts.tv_sec++;
            ts.tv_nsec = (ts.tv_nsec + (wait * 1000)) - NANOSECONDS_IN_SECOND;
        }
        else {
            ts.tv_nsec += (wait * 1000);
        }
        
        /* Wait specified amount of time or for receiving thread to signal us */
        pthread_cond_timedwait(&cond_packet_change, &mtx_cond_packet_change, &ts);        
        pthread_mutex_unlock(&mtx_cond_packet_change);
    }
    
    log_line("SERV: Sending thread terminated.", LOG_ALWAYS);
    
    pthread_exit(NULL);
}
