#ifndef COM_H
#define	COM_H

#include <time.h>
#include "client.h"

/*Total number of clients */
extern unsigned int num_connections;

/*Sent bytes counter */
extern unsigned int sent_bytes;
/*Sent dgrams counter */
extern unsigned int sent_dgrams;
/*Reveived bytes counter */
extern unsigned int recv_bytes;
/*Received dgrams counter */
extern unsigned int recv_dgrams;


typedef struct {
    /* Packet ID */
    int seq_id;
    /* Packet's message */
    char *message;
    /* Raw message */
    char *raw_msg;
    /* Client address */
    struct sockaddr_in *addr;
    /* Packet timestamp */
    struct timeval timestamp;
    /* Packet state. 0-new. 1-sent and waiting for ack*/
    unsigned short state;
    /* ACK requirement*/
    unsigned short req_ack;
    
} packet_t;

void enqueue_dgram(client_t *client, char *msg, int req_ack);
void add_packet_mesasge(packet_t *pkt);
void send_packet(packet_t *pkt, client_t *client);
int packet_timestamp_old(packet_t pkt, int *wait);
int client_timestamp_timeout(client_t *client);
int client_timestamp_remove(client_t *client);
void send_ack(client_t *client, int seq_id, int resend);
void recv_ack(client_t *client, int seq_id);
void inform_server_full(struct sockaddr_in *addr);
void broadcast_clients(char *msg);

#endif	/* COM_H */

