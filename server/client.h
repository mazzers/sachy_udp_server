#ifndef CLIENT_H
#define CLIENT_H
/*Client ID len*/
#define CODE_LEN 4
/*Client's colors*/
#define COLOR_WHITE 1
#define COLOR_BLACK 2

#include <sys/time.h>

#include "queue.h"
#include "global.h"


extern char *client_code[MAX_CURRENT_CLIENTS];
extern unsigned int client_num;

typedef struct {
	/*Client address*/
	struct sockaddr_in *addr;
	/*CLient addr as string*/
	char *addr_str;
	/*Client index*/
	int client_index;
	/*Client game index*/
	int game_index;
	/*Client mutex*/
	pthread_mutex_t mtx_client;
	/*Client outgoing sequence*/
	int pkt_send_seq_id;
	/*Client incoming sequence*/
	int pkt_recv_seq_id;
	/*Last activity timestamp*/
	struct timeval timestamp;
	/*Client state*/
	unsigned short state;
	/*Client ID*/
	char *code;
	/*Client queue of messages*/
	Queue *dgram_queue;
	/*Client color*/
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
int get_client_index_by_code(char *code);
void send_client_code(client_t *client);
int generate_client_code(char *s, int iteration);
void set_client_color(client_t *client, int color);




#endif