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
 * File: com.c
 * Description: Is responsible for all communication with connected clients
 *              (sending datagrams, receiving and sending ACKs..)
 * 
 * -----------------------------------------------------------------------------
 * 
 * @author: Martin Kucera, 2014
 * @version: 1.02
 * 
 */

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

/* Number of sent bytes */
unsigned int sent_bytes = 0;
/* Number of sent datagrams */
unsigned int sent_dgrams = 0;
/* Number of received bytes */
unsigned int recv_bytes = 0;
/* Number of received datagrams */
unsigned int recv_dgrams = 0;
/* Number of client connections (total) */
unsigned int num_connections = 0;

/* Logger buffer */
char log_buffer[LOG_BUFFER_SIZE];

/**
 * void enqueue_dgram(client_t *client, char *msg, int req_ack)
 * 
 * Inserts new datagram with message passed as argument to given client.
 * Datagram may or may not need an ACK packet. If client's outgoing queue
 * is empty, datagram is send immediately without waiting for sender thread
 * to pick him up.
 */
void enqueue_dgram(client_t *client, char *msg, int req_ack) {
    //printf("com.c: enqueue_dgram\n");
    packet_t *packet;
    int sent_immediately = 0;
    
    if(client != NULL) {
        sprintf(log_buffer,
                "Enqueueing packet with message: %s",
                msg
                );
       
        
        log_line(log_buffer, LOG_DEBUG);
        
        packet = (packet_t *) malloc(sizeof(packet_t));
        
        /* Set packet's state to new */
        packet->state = 0;
        /* Set packet's destination */
        packet->addr = client->addr;
        /* Set packet's req ACK flag */
        packet->req_ack = req_ack;
        
        /* Make copy of message */
        packet->msg = (char *) malloc(strlen(msg) + 1);
        strcpy(packet->msg, msg);
                
        /* Check if outgoing queue is empty, if so, send packet immediately */
        if(queue_size(client->dgram_queue) == 0) {            
            send_packet(packet, client);
            sent_immediately = 1;
        }
        
        if(sent_immediately && !req_ack) {
            free(packet->msg);
            free(packet->payload);
            free(packet);
        }
        else {
            /* Add packet to client's dgram queue */
            queue_push(client->dgram_queue, (void *) packet);
        }
    }
}

/*
 * void build_packet_payload(packet_t *pkt)
 * 
 * Builds packet payload using structure:
 * APP_TOKEN;SEQ_ID;MESSAGE
 */
void build_packet_payload(packet_t *pkt) {
    //printf("com.c: build_packet_payload\n");
    char *payload;
    char seq_id_buff[11];
    int len;

    sprintf(seq_id_buff, "%u", pkt->seq_id);
    len = strlen(STRINGIFY(APP_TOKEN)) + strlen(seq_id_buff)
            + strlen(pkt->msg) + 4;

    payload = (char *) malloc(len);
    
    sprintf(payload,
            "%s;%u;%s",
            STRINGIFY(APP_TOKEN),
            pkt->seq_id,
            pkt->msg
            );

    pkt->payload = payload;
}

/*
 * void send_packet(packet_t *pkt, client_t *client)
 * 
 * Sends packet immediately to destination
 */
void send_packet(packet_t *pkt, client_t *client) {
    //printf("com.c: send_packet\n");
    if(!pkt->state) {
        pkt->seq_id = client->pkt_send_seq_id;
    }
    
    /* If packet will be ACKd, increment send id */
    if(!pkt->state && pkt->req_ack) {
        client->pkt_send_seq_id++;
    }
    
    if(!pkt->state) {
        build_packet_payload(pkt);
    }
    
    /* Mark packet as waiting for ACK */
    pkt->state = 1;
    /* Set packet timestamp */
    gettimeofday(&pkt->timestamp, NULL);
    
    sendto(server_sockfd, pkt->payload, strlen(pkt->payload), 0, (struct sockaddr*) pkt->addr, sizeof(*pkt->addr));
    
 
    sprintf(log_buffer,
            "DATA_OUT: %s ---> %s:%d",
            pkt->payload,
	    client->addr_str,
	    htons(client->addr->sin_port)
            );
    
    log_line(log_buffer, LOG_DEBUG);
    
    /* Stats */
    sent_bytes += strlen(pkt->payload);
    sent_dgrams++;
}

/**
 * int packet_timestamp_old(packet_t pkt, int *wait)
 * 
 * Checks if timestamp of given packet is older than maximum allowed time
 * for packet marked as sent and requiring ACK before timeout. 
 * 
 * If packet isn't timeouted yet and wait isn't NULL, compares value of wait
 * and current timeout. If current timeout is lower than wait, updates wait.
 */
int packet_timestamp_old(packet_t pkt, int *wait) {
    //printf("com.c: packet_timestamp_old\n");
    int cur_wait = 0;
    struct timeval cur_tv;
    gettimeofday(&cur_tv, NULL);

    /* Longer than second */
    if(cur_tv.tv_sec - pkt.timestamp.tv_sec >= 1) {
        sprintf(log_buffer,
                "Packet with payload %s timeouted",
                pkt.payload
                );
     
        log_line(log_buffer, LOG_DEBUG);
        
        return 1;
    }

    cur_wait = MAX_PACKET_AGE_USEC - (cur_tv.tv_usec - pkt.timestamp.tv_usec);

    /* If current difference is smaller */
    if(cur_wait > 0 && cur_wait < (*wait)) {
        (*wait) = cur_wait;
    }
    
    /* Debug */
    if(cur_wait <= 0) {
        sprintf(log_buffer,
                "Packet with payload %s and SEQ_ID %d timeouted",
                pkt.payload,
                pkt.seq_id
                );
     
        log_line(log_buffer, LOG_DEBUG);
    }

    return (cur_wait <= 0);
}

/**
 * int client_timestamp_timeout(client_t *client)
 * 
 * Checks if client's timestamp of last communication exceeded the maximum
 * value specified by MAX_CLIENT_NORESPONSE_SEC
 */
int client_timestamp_timeout(client_t *client) {
    //printf("com.c client_timestamp_timeout\n");
    struct timeval cur_tv;
    gettimeofday(&cur_tv, NULL);

    return ( ( cur_tv.tv_sec - (client)->timestamp.tv_sec > MAX_CLIENT_NORESPONSE_SEC ) );
}

int client_timestamp_remove(client_t *client) {
    //printf("com.c: client_timestamp_remove\n");
    struct timeval cur_tv;
    gettimeofday(&cur_tv, NULL);

    return ( ( cur_tv.tv_sec - (client)->timestamp.tv_sec > MAX_CLIENT_TIMEOUT_SEC ) );
}


/**
 * void send_ack(client_t *client, int seq_id, int resend)
 * 
 * Sends ACK packet to client. If resend is 0, increases client's
 * RECV_SEQ_ID
 */
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
        
        
        buff = (char *) malloc((8 + len + strlen(STRINGIFY(APP_TOKEN))));
        
        sprintf(buff,
                "%s;%d;ACK;%d;",
                STRINGIFY(APP_TOKEN),
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
            printf("recv increased\n");
            client->pkt_recv_seq_id++;
            printf("%d\n",client->pkt_recv_seq_id );
        }
        
        /* Update client's timestamp */
        update_client_timestamp(client);
        
        /* Stats */
        sent_bytes += strlen(buff);
        sent_dgrams++;
        
        free(buff);
    }
}


/**
 * void recv_ack(client_t *client, int seq_id)
 * 
 * Processes incoming ACK packet. Checks if client's packet at front of
 * his outgoing queue has matching SEQ_ID, if so, removes packet from queue and
 * checks if there are any more packets waiting. If there are, notifies the
 * sender thread to wake up.
 */
void recv_ack(client_t *client, int seq_id) {
    
    packet_t *packet;
    
    if(client != NULL) {
        if(queue_size(client->dgram_queue) > 0) {
            packet = queue_front(client->dgram_queue);
            
            if(packet->seq_id == seq_id) {            
                /* Log */
                sprintf(log_buffer,
                        "ACK waiting packet with payload %s and SEQ_ID %d",
                        packet->payload,
                        packet->seq_id
                        );
                
                log_line(log_buffer, LOG_DEBUG);
                
                queue_pop(client->dgram_queue, 0);
                
                free(packet->payload);
                free(packet->msg);
                free(packet);
                
                /* If client has any more queued packets, signal
                 * sender thread
                 */
                if(queue_size(client->dgram_queue) > 0) {
                    pthread_cond_signal(&cond_packet_change);
                }
            }
        }
    }
}

/**
 * void inform_server_full(struct sockaddr_in *addr)
 * 
 * Notifies client with given address that server is currently full.
 */
void inform_server_full(struct sockaddr_in *addr) {
    printf("com.c inform_server_full\n");
    char *buff = (char *) malloc(strlen(STRINGIFY(APP_TOKEN)) + 15);
    char addr_str[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &addr->sin_addr, addr_str, INET_ADDRSTRLEN);
    
    sprintf(buff,
            "%s;1;SERVER_FULL",
            STRINGIFY(APP_TOKEN)
            );
    
    sendto(
            server_sockfd,
            buff,
            strlen(buff),
            0,
            (struct sockaddr *) addr,
            sizeof(*addr)
    );
    
    /* Log */
    sprintf(log_buffer,
            "DATA_OUT: %s ---> %s:%d",
            buff,
	    addr_str,
	    htons(addr->sin_port)
            );
    
    log_line(log_buffer, LOG_DEBUG);
    
    /* Stats */
    sent_bytes += strlen(buff);
    sent_dgrams++;
    
    sprintf(buff,
            "%s;1;ACK;1",
            STRINGIFY(APP_TOKEN)
            );
    
    sendto(
            server_sockfd,
            buff,
            strlen(buff),
            0,
            (struct sockaddr *) addr,
            sizeof(*addr)
    );
    
    /* Log */
    sprintf(log_buffer, 
            "DATA_OUT: %s ---> %s:%d",
            buff,
            addr_str,
	    htons(addr->sin_port)
            );
    
    log_line(log_buffer, LOG_DEBUG);
    
    /* Stats */
    sent_bytes += strlen(buff);
    sent_dgrams++;
    
    free(buff);
}

/**
 * void broadcast_clients(char *msg)
 * 
 * Sends message to all connected clients
 */
void broadcast_clients(char *msg) {
    printf("com.c: broadcast_clients\n");
    int i = 0;
    client_t *client;
    char *buff = (char *) malloc(strlen(msg) + strlen(STRINGIFY(APP_TOKEN)) + 13);
    
    for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
        client = get_client_by_index(i);
        
        if(client) {
            sprintf(buff,
                    "%s;%d;%s",
                    STRINGIFY(APP_TOKEN),
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
            
            /* Log */
            sprintf(log_buffer,
                    "DATA_OUT: %s ---> %s:%d",
                    buff,
		    client->addr_str,
		    htons(client->addr->sin_port)
                    );
            
            log_line(log_buffer, LOG_DEBUG);
            
            /* Release client */
            release_client(client);
        }
    }
    
    free(buff);
}
