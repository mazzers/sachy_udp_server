#ifndef CLIENT_H
#define CLIENT_H

extern unsigned int client_num;

typedef struct {

	struct sockaddr_in *addr;
	int client_index;
	int game_index;




} client_t;

void add_client(struct sockaddr_in *addr);




#endif