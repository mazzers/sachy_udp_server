#ifndef CLIENT_H
#define CLIENT_H

extern unsigned int client_num;

typedef struct {

	struct sockaddr_in *addr;
	char *addr_str;
	int client_index;
	int game_index;
	pthread_mutex_t mtx_client;




} client_t;

void add_client(struct sockaddr_in *addr);
client_t* get_client_by_addr(struct sockaddr_in *addr);
void release_client(client_t *client);




#endif