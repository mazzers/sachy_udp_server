#ifndef CLIENT_H
#define CLIENT_H

#define RECONNECT_CODE_LEN 4

#include <sys/time.h>

#include "queue.h"
#include "global.h"


extern char *reconnect_code[MAX_CURRENT_CLIENTS];
extern unsigned int client_num;

typedef struct {

	struct sockaddr_in *addr;
	char *addr_str;
	int client_index;
	int game_index;
	pthread_mutex_t mtx_client;
	int pkt_send_seq_id;
	int pkt_recv_seq_id;
	struct timeval timestamp;
	unsigned short state;
	char *reconnect_code;
	Queue *dgram_queue;
	int color;




} client_t;

void add_client(struct sockaddr_in *addr);
client_t* get_client_by_addr(struct sockaddr_in *addr);
void release_client(client_t *client);
void update_client_timestamp(client_t *client);
void remove_client(client_t **client);
void clear_client_dgram_queue(client_t *client);
void clear_all_clients();
client_t* get_client_by_index(int index);
int get_client_index_by_rcode(char *code);
void send_reconnect_code(client_t *client);
int generate_reconnect_code(char *s, int iteration);




#endif