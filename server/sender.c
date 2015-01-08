
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <netinet/in.h>

#include "global.h"
#include "client.h"
#include "com.h"
#include "game.h"
#include "logger.h"

pthread_cond_t cond_packet_change;

pthread_mutex_t mtx_cond_packet_change;

/*
___________________________________________________________

    Sender thread. Send packets to clients. Resend in case
    if ACK wasn't reveived. Delete inactive clients.
___________________________________________________________
*/
void *start_sending(void *arg) {

    pthread_mutex_t *thr_mutex = (pthread_mutex_t *) arg;

    int wait;

    int i;

    client_t *client;

    packet_t *packet;

    struct timespec ts;

    unsigned int got_clients;
    
    while(!stop_thread(thr_mutex)) {
        got_clients = 0;
        wait = -1;
        
        for(i = 0; i < MAX_CURRENT_CLIENTS && got_clients <= client_num; i++) {

            client = get_client_by_index(i);
            
            if(client) {
                got_clients++;

                if(client->state) {

                    if(client_timestamp_timeout(client)) {
                        

                        timeout_game(client);
                        client->state = 0;
                        
                    } 
                    else if(queue_size(client->dgram_queue) > 0) {
                        packet = queue_front(client->dgram_queue);


                        while( packet &&  ( ( packet->state == 0) || 
                                ( packet->state == 1 && packet_timestamp_old(*packet, &wait)))) {
                            
                            send_packet(packet, client);
                            
                            if(!packet->req_ack) {
                                queue_pop(client->dgram_queue, 0);
                                
                                free(packet->raw_msg);
                                free(packet->message);
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

                        leave_game(client);
                    }

                    remove_client(&client);

                }
                
                if(client) {
                    release_client(client);
                }
            }
        }
        

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
        

        pthread_cond_timedwait(&cond_packet_change, &mtx_cond_packet_change, &ts);        
        pthread_mutex_unlock(&mtx_cond_packet_change);
    }
    
    log_line("SERV: Sending thread terminated.", LOG_ALWAYS);
    
    pthread_exit(NULL);
}
