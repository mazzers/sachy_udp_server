#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include "client.h"
#include "com.h"
#include "global.h"
#include "server.h"
#include "sender.h"
#include "logger.h"

/*Sent bytes counter */
unsigned int sent_bytes = 0;
/*Sent dgrams counter */
unsigned int sent_dgrams = 0;
/*Reveived bytes counter */
unsigned int recv_bytes = 0;
/*Received dgrams counter */
unsigned int recv_dgrams = 0;
/*Total number of clients */
unsigned int num_connections = 0;

/* Log buffer */
char log_buffer[LOG_BUFFER_SIZE];

/*
___________________________________________________________

    Add new packet to client's queue. 
    Send immediately if queue if empty.
___________________________________________________________
*/
void enqueue_dgram(client_t *client, char *msg, int req_ack) {
    packet_t *packet;
    int sent_immediately = 0; 
    if(client != NULL) {
        sprintf(log_buffer,
                "Enqueueing packet with message: %s",
                msg
                );
       
        
        log_line(log_buffer, LOG_DEBUG);
        
        packet = (packet_t *) malloc(sizeof(packet_t));
        
        packet->state = 0;
        packet->addr = client->addr;
        packet->req_ack = req_ack;

        packet->raw_msg = (char *) malloc(strlen(msg) + 1);
        strcpy(packet->raw_msg, msg);

        if(queue_size(client->dgram_queue) == 0) {            
            send_packet(packet, client);
            sent_immediately = 1;
        }
        
        if(sent_immediately && !req_ack) {
            free(packet->raw_msg);
            free(packet->message);
            free(packet);
        }
        else {
            queue_push(client->dgram_queue, (void *) packet);
        }
    }
}

/*
___________________________________________________________

    Create new packet.
___________________________________________________________
*/
void add_packet_message(packet_t *packet) {
    char *message;
    char seq_id_buff[11];
    int len;

    sprintf(seq_id_buff, "%u", packet->seq_id);
    len = strlen(STRINGIFY(TOKEN)) + strlen(seq_id_buff)
            + strlen(packet->raw_msg) + 4;

    message = (char *) malloc(len);
    
    sprintf(message,
            "%s;%u;%s",
            STRINGIFY(TOKEN),
            packet->seq_id,
            packet->raw_msg
            );

    packet->message = message;
}


void send_packet(packet_t *packet, client_t *client) {

    if(!packet->state) {
        packet->seq_id = client->pkt_send_seq_id;
    }
    
    if(!packet->state && packet->req_ack) {
        client->pkt_send_seq_id++;
    }
    
    if(!packet->state) {
        add_packet_message(packet);
    }
    

    packet->state = 1;
    gettimeofday(&packet->timestamp, NULL);
    
    sendto(server_sockfd, packet->message, strlen(packet->message), 0, (struct sockaddr*) packet->addr, sizeof(*packet->addr));
    
 
    sprintf(log_buffer,
            "DATA_OUT: %s ---> %s:%d",
            packet->message,
	    client->addr_str,
	    htons(client->addr->sin_port)
            );
    
    log_line(log_buffer, LOG_DEBUG);
    

    sent_bytes += strlen(packet->message);
    sent_dgrams++;
}


int packet_timestamp_old(packet_t packet, int *wait) {
    int cur_wait = 0;
    struct timeval cur_tv;
    gettimeofday(&cur_tv, NULL);

    if(cur_tv.tv_sec - packet.timestamp.tv_sec >= 1) {
        sprintf(log_buffer,
                "Packet with message %s timeouted",
                packet.message
                );
     
        log_line(log_buffer, LOG_DEBUG);
        
        return 1;
    }

    cur_wait = MAX_PACKET_AGE_USEC - (cur_tv.tv_usec - packet.timestamp.tv_usec);

    if(cur_wait > 0 && cur_wait < (*wait)) {
        (*wait) = cur_wait;
    }
    
    if(cur_wait <= 0) {
        sprintf(log_buffer,
                "Packet with message %s and ID %d timeouted",
                packet.message,
                packet.seq_id
                );
     
        log_line(log_buffer, LOG_DEBUG);
    }

    return (cur_wait <= 0);
}


int client_timestamp_timeout(client_t *client) {
    struct timeval cur_tv;
    gettimeofday(&cur_tv, NULL);

    return ( ( cur_tv.tv_sec - (client)->timestamp.tv_sec > MAX_CLIENT_NORESPONSE_SEC ) );
}

int client_timestamp_remove(client_t *client) {

    struct timeval cur_tv;
    gettimeofday(&cur_tv, NULL);

    return ( ( cur_tv.tv_sec - (client)->timestamp.tv_sec > MAX_CLIENT_TIMEOUT_SEC ) );
}


void send_ack(client_t *client, int seq_id, int resend) {
    
    char *buff;
    int len;
    
    if(client != NULL) {
        buff = (char *) malloc(11);
        
        sprintf(buff, "%d", seq_id);
        len = strlen(buff);
        sprintf(buff, "%d", client->pkt_send_seq_id);
        len += strlen(buff);
        
        free(buff);
        
        
        buff = (char *) malloc((8 + len + strlen(STRINGIFY(TOKEN))));
        
        sprintf(buff,
                "%s;%d;ACK;%d;",
                STRINGIFY(TOKEN),
                client->pkt_send_seq_id,
                seq_id
                );
        
        sendto(
                server_sockfd,
                buff,
                strlen(buff),
                0,
                (struct sockaddr *) client->addr,
                sizeof(*client->addr)
        );
        
        sprintf(log_buffer,
                "DATA_OUT: %s ---> %s:%d",
                buff,
		client->addr_str,
		htons(client->addr->sin_port)
                );
 
        log_line(log_buffer, LOG_DEBUG);
        
        if(!resend) {
            client->pkt_recv_seq_id++;
            
        }
        

        update_client_timestamp(client);
        

        sent_bytes += strlen(buff);
        sent_dgrams++;
        
        free(buff);
    }
}



void recv_ack(client_t *client, int seq_id) {
    
    packet_t *packet;
    
    if(client != NULL) {
        if(queue_size(client->dgram_queue) > 0) {
            packet = queue_front(client->dgram_queue);
            
            if(packet->seq_id == seq_id) {            
                /* Log */
                sprintf(log_buffer,
                        "ACK waiting packet with message %s and ID %d",
                        packet->message,
                        packet->seq_id
                        );
                
                log_line(log_buffer, LOG_DEBUG);
                
                queue_pop(client->dgram_queue, 0);
                
                free(packet->message);
                free(packet->raw_msg);
                free(packet);
                
              
                if(queue_size(client->dgram_queue) > 0) {
                    pthread_cond_signal(&cond_packet_change);
                }
            }
        }
    }
}

void inform_server_full(struct sockaddr_in *addr) {
    
    char *buff = (char *) malloc(strlen(STRINGIFY(TOKEN)) + 15);
    char addr_str[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &addr->sin_addr, addr_str, INET_ADDRSTRLEN);
    
    sprintf(buff,
            "%s;1;SERVER_FULL",
            STRINGIFY(TOKEN)
            );
    
    sendto(
            server_sockfd,
            buff,
            strlen(buff),
            0,
            (struct sockaddr *) addr,
            sizeof(*addr)
    );
    

    sprintf(log_buffer,
            "DATA_OUT: %s ---> %s:%d",
            buff,
	    addr_str,
	    htons(addr->sin_port)
            );
    
    log_line(log_buffer, LOG_DEBUG);
    

    sent_bytes += strlen(buff);
    sent_dgrams++;
    
    sprintf(buff,
            "%s;1;ACK;1",
            STRINGIFY(TOKEN)
            );
    
    sendto(
            server_sockfd,
            buff,
            strlen(buff),
            0,
            (struct sockaddr *) addr,
            sizeof(*addr)
    );

    sprintf(log_buffer, 
            "DATA_OUT: %s ---> %s:%d",
            buff,
            addr_str,
	    htons(addr->sin_port)
            );
    
    log_line(log_buffer, LOG_DEBUG);

    sent_bytes += strlen(buff);
    sent_dgrams++;
    
    free(buff);
}


void broadcast_clients(char *msg) {
    
    int i = 0;
    client_t *client;
    char *buff = (char *) malloc(strlen(msg) + strlen(STRINGIFY(TOKEN)) + 13);
    
    for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
        client = get_client_by_index(i);
        
        if(client) {
            sprintf(buff,
                    "%s;%d;%s",
                    STRINGIFY(TOKEN),
                    client->pkt_send_seq_id,
                    msg
                    );
            
            sendto(
                    server_sockfd,
                    buff,
                    strlen(buff),
                    0,
                    (struct sockaddr *) client->addr,
                    sizeof(*(client->addr))
            );
            

            sprintf(log_buffer,
                    "DATA_OUT: %s ---> %s:%d",
                    buff,
		    client->addr_str,
		    htons(client->addr->sin_port)
                    );
            
            log_line(log_buffer, LOG_DEBUG);
            

            release_client(client);
        }
    }
    
    free(buff);
}
