#ifndef CLIENT_H
#define CLIENT_H

#define CODE_LEN 4
#define NAME_LEN 17

#define COLOR_WHITE 1
#define COLOR_BLACK 2

#include <sys/time.h>

#include "queue.h"
#include "global.h"


extern char *client_code[MAX_CURRENT_CLIENTS];
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
	char *code;
	Queue *dgram_queue;
	int color;
	char *name;




} client_t;

void add_client(struct sockaddr_in *addr);
client_t* get_client_by_addr(struct sockaddr_in *addr);
void release_client(client_t *client);
void update_client_timestamp(client_t *client);
void remove_client(client_t **client);
void clear_client_dgram_queue(client_t *client);
void clear_all_clients();
client_t* get_client_by_index(int index);
int get_client_index_by_code(char *code);
void send_client_code(client_t *client);
//void send_client_name(client_t *client);
int generate_client_code(char *s, int iteration);
void set_client_color(client_t *client, int color);




#endif