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

#ifndef COM_H
#define	COM_H

#include <time.h>

#include "client.h"

/* Number of sent bytes */
extern unsigned int sent_bytes;
/* Number of sent datagrams */
extern unsigned int sent_dgrams;
/* Number of received bytes */
extern unsigned int recv_bytes;
/* Number of received datagrams */
extern unsigned int recv_dgrams;
/* Number of client connections (total) */
extern unsigned int num_connections;

typedef struct {
    /* Packet sequential ID */
    int seq_id;
    /* Packet's payload */
    char *payload;
    /* Raw message */
    char *msg;
    /* Destination address */
    struct sockaddr_in *addr;
    /* Last communication timestamp */
    struct timeval timestamp;
    /* State - 0 new, 1 waiting for ACK */
    unsigned short state;
    /* Flag indicating if packet requires ACK */
    unsigned short req_ack;
    
} packet_t;

/* Function prototypes */
void enqueue_dgram(client_t *client, char *msg, int req_ack);
void build_packet_payload(packet_t *pkt);
void send_packet(packet_t *pkt, client_t *client);
int packet_timestamp_old(packet_t pkt, int *wait);
int client_timestamp_timeout(client_t *client);
int client_timestamp_remove(client_t *client);
void send_ack(client_t *client, int seq_id, int resend);
void recv_ack(client_t *client, int seq_id);
void inform_server_full(struct sockaddr_in *addr);
void broadcast_clients(char *msg);

#endif	/* COM_H */

