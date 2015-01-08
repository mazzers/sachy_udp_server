#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "client.h"
#include "client.h"
#include "queue.h"
#include "com.h"
#include "logger.h"
#include "game.h"
#include "server.h"

/*Array of clients*/
client_t *clients[MAX_CURRENT_CLIENTS]={NULL};
/*Array of client id*/
char *client_code[MAX_CURRENT_CLIENTS] = {NULL};
/*Client count*/
unsigned int client_num=0;
/*Log buffer*/
char log_buffer[LOG_BUFFER_SIZE];

/*
___________________________________________________________

    Add new client. Place him into clients array.
    Create client's packet queue. 
___________________________________________________________
*/
void add_client(struct sockaddr_in *addr){
	client_t *new_client;
	client_t *existing_client;
	struct sockaddr_in *new_addr;
	int i;

	if (client_num < MAX_CURRENT_CLIENTS)
	{
		existing_client = get_client_by_addr(addr);
		if (existing_client==NULL)
		{
			new_client = (client_t *) malloc(sizeof(client_t));
			new_addr = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));

			/*Allocates all needed memory and defines new client values*/

			memcpy(new_addr, addr, sizeof(struct sockaddr_in));
			new_client->addr = new_addr;
			new_client->game_index=-1;
			new_client->addr_str = malloc(INET_ADDRSTRLEN +1);
			new_client->pkt_recv_seq_id = 1;
			new_client->pkt_send_seq_id = 1;
			new_client->state = 1;
			new_client->color =0;
			new_client->code = (char *) malloc(CODE_LEN + 1);
			new_client->dgram_queue = malloc(sizeof(Queue));
			
			/*Initiate client's mutex*/

			pthread_mutex_init(&new_client->mtx_client, NULL);
			queue_init(new_client->dgram_queue);

			inet_ntop(AF_INET, &addr->sin_addr, new_client->addr_str, INET_ADDRSTRLEN);
			/*Set up client timestamp*/
			update_client_timestamp(new_client);

			/*Place new client to clients array*/

			for(i=0;i<MAX_CURRENT_CLIENTS;i++){
				if(clients[i]==NULL){
					new_client->client_index=i;
					clients[i]=new_client;
					
					client_num++;
					
					break;
				}



			}
			/*Generate client id*/
			generate_client_code(new_client->code, 0);
			client_code[new_client->client_index] = new_client->code;

			sprintf(log_buffer,
				"New client. IP: %s port: %d.",
				new_client->addr_str,
				htons(addr->sin_port)
				);

			log_line(log_buffer, LOG_INFO);

            /* Increase number of connected clients */
			num_connections++;
		}else{
			release_client(existing_client);


		}
		
	}else{
		printf("New clients can't connect. Server has reached clients limit.\n");
	}
}

/*
___________________________________________________________

    Return client by inet address.
___________________________________________________________
*/
client_t* get_client_by_addr(struct sockaddr_in *addr) {
	int i = 0;
	char addr_str[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &(addr)->sin_addr, addr_str, INET_ADDRSTRLEN);

	for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
		if(clients[i] != NULL) {
			pthread_mutex_lock(&clients[i]->mtx_client);

            /* Check if client exists */
			if(clients[i] != NULL) {
                    /* Check adress and port */
				if(strncmp(clients[i]->addr_str, addr_str, INET_ADDRSTRLEN) == 0
					&& (htons((addr)->sin_port) == htons(clients[i]->addr->sin_port) )) {
					return clients[i];

			}else{
			}
		}
		release_client(clients[i]);
	}
}

return NULL;
}

/*
___________________________________________________________

    Release client's mutex.
___________________________________________________________
*/
void release_client(client_t *client) {   
	if(client && pthread_mutex_trylock(&client->mtx_client) != 0) {
		pthread_mutex_unlock(&client->mtx_client);
	}
	else {
		log_line("Wasn't able to unlock client", LOG_WARN);
	}
}

/*
___________________________________________________________

    Return client by index.
___________________________________________________________
*/
client_t* get_client_by_index(int index) {
	if(index>= 0 && MAX_CURRENT_CLIENTS > index && clients[index]) {    
		pthread_mutex_lock(&clients[index]->mtx_client);

		return clients[index];
	}

	return NULL;
}

/*
___________________________________________________________

    Update time of client's last activity.
___________________________________________________________
*/
void update_client_timestamp(client_t *client) {
	if(client != NULL) {
		gettimeofday(&client->timestamp, NULL);
	}
}

/*
___________________________________________________________

    Remove certain client.
___________________________________________________________
*/
void remove_client(client_t **client) {          
	if(client != NULL) {
		sprintf(log_buffer,
			"Removing client with IP address: %s and port %d",
			(*client)->addr_str,
			htons((*client)->addr->sin_port)
			);

		log_line(log_buffer, LOG_INFO);

		clients[(*client)->client_index] = NULL;
		client_code[(*client)->client_index] = NULL;

		free((*client)->addr);
		free((*client)->addr_str);
		free((*client)->code);
		client_num --;

		release_client((*client));

		clear_client_dgram_queue((*client));
		free((*client)->dgram_queue);
		free((*client));
	}

	*client = NULL;
}

/*
___________________________________________________________

    Clear client's packet queue.
___________________________________________________________
*/
void clear_client_dgram_queue(client_t *client) {
	packet_t *packet = queue_front(client->dgram_queue);

	while(packet) {
		queue_pop(client->dgram_queue, 0);

		if(packet->state) {
			free(packet->message);
		}

		free(packet->raw_msg);
		free(packet);

		packet = queue_front(client->dgram_queue);
	}
}

/*
___________________________________________________________

    Remove all clients.
___________________________________________________________
*/
void clear_all_clients() {
	int i = 0;
	client_t *client;

	for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
		client = get_client_by_index(i);

		if(client) {
			remove_client(&client);
		}
	}
}

/*
___________________________________________________________

    Generate client's code.
___________________________________________________________
*/
int generate_client_code(char *s, int iteration)  {

	int existing_index;

	if(iteration > 100) {
		s[0] = 0;

		return 0;
	}

	gen_random(s, CODE_LEN);
	existing_index = get_client_index_by_code(s);

	if(existing_index != -1) {
		generate_client_code(s, iteration++);
	}

	return 1;
}

/*
___________________________________________________________

    Send client's code.
___________________________________________________________
*/
void send_client_code(client_t *client) {
	char *buff = (char *) malloc(11 + CODE_LEN);

	sprintf(buff,
		"CLIENT_CODE;%s;",
		client->code
		);
	enqueue_dgram(client, buff, 1);

	free(buff);
}

/*
___________________________________________________________

    Get client index by code.
___________________________________________________________
*/
int get_client_index_by_code(char *code) {
	int i;

	for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
		if(client_code[i] && strncmp(client_code[i], code, CODE_LEN) == 0) {
			return i;
		}
	}

	return -1;
}

/*
___________________________________________________________

    Setting up client's color.
___________________________________________________________
*/
void set_client_color(client_t *client, int color){
	if(color==COLOR_WHITE){
		client->color=COLOR_WHITE;
	}else if(color==COLOR_BLACK){
		client->color=COLOR_BLACK;
	}else{
		client->color=0;
	}
}
