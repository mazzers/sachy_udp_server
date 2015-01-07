#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "client.h"
#include "client.h"
#include "queue.h"
#include "com.h"
#include "logger.h"
#include "game.h"
#include "server.h"

//clients array
client_t *clients[MAX_CURRENT_CLIENTS]={NULL};
char *client_code[MAX_CURRENT_CLIENTS] = {NULL};

unsigned int client_num=0;
char log_buffer[LOG_BUFFER_SIZE];

void add_client(struct sockaddr_in *addr){
	//printf("Client.c: add_client\n");
	client_t *new_client;
	client_t *existing_client;
	struct sockaddr_in *new_addr;
	int i;

	if (client_num < MAX_CURRENT_CLIENTS)
	{
		//printf("client_num < MAX_CURRENT_CLIENTS\n");
		existing_client = get_client_by_addr(addr);
		//printf("existing_client = get_client_by_addr(addr)\n");
		if (existing_client==NULL)
		{

			//printf("client.c: it is new client\n");
			new_client = (client_t *) malloc(sizeof(client_t));

			new_addr = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));

			memcpy(new_addr, addr, sizeof(struct sockaddr_in));
			new_client->addr = new_addr;
			new_client->game_index=-1;
			new_client->addr_str = malloc(INET_ADDRSTRLEN +1);
			new_client->pkt_recv_seq_id = 1;
			new_client->pkt_send_seq_id = 1;
			new_client->state = 1;
			new_client->color =0;
			// new_client->name = malloc(50*sizeof(char));
			// sprintf(new_client -> name,"%s",name);
			//new_client->name = (char *) malloc(NAME_LEN+1);
			new_client->code = (char *) malloc(CODE_LEN + 1);
			new_client->dgram_queue = malloc(sizeof(Queue));

			pthread_mutex_init(&new_client->mtx_client, NULL);
			queue_init(new_client->dgram_queue);

			inet_ntop(AF_INET, &addr->sin_addr, new_client->addr_str, INET_ADDRSTRLEN);
			update_client_timestamp(new_client);

			for(i=0;i<MAX_CURRENT_CLIENTS;i++){
				if(clients[i]==NULL){
					new_client->client_index=i;
					clients[i]=new_client;
					
					client_num++;
					
					break;
				}



			}
			generate_client_code(new_client->code, 0);
            client_code[new_client->client_index] = new_client->code;

            sprintf(log_buffer,
                    "Added new client with IP address: %s and port %d. Client's name:%s",
                    new_client->addr_str,
                    htons(addr->sin_port),
                    new_client->name
                    );
            
            log_line(log_buffer, LOG_INFO);
            
            /* Stats */
            num_connections++;
		}else{
			// printf("client.c: client exists\n");
			release_client(existing_client);


		}
		
	}else{
		printf("New client tried to connect but server is full\n");
	}
}



client_t* get_client_by_addr(struct sockaddr_in *addr) {
	//printf("Client.c: get_client_by_addr\n");
	int i = 0;
	char addr_str[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &(addr)->sin_addr, addr_str, INET_ADDRSTRLEN);

	for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
		//printf("looking\n");
		if(clients[i] != NULL) {
			//printf("b4 lock\n");
			pthread_mutex_lock(&clients[i]->mtx_client);
			//printf("lock\n");
			

            /* Check if client still exists */
			if(clients[i] != NULL) {
				
                    /* Check if address and port matches */
				if(strncmp(clients[i]->addr_str, addr_str, INET_ADDRSTRLEN) == 0
					&& (htons((addr)->sin_port) == htons(clients[i]->addr->sin_port) )) {
					// printf("client matched\n");
				return clients[i];

			}else{
				//rintf("client not matched\n");
			}
		}
		// printf("call release_client\n");
		release_client(clients[i]);
	}
}

return NULL;
}

void release_client(client_t *client) { 
	//printf("Client.c: release_client\n");   
    if(client && pthread_mutex_trylock(&client->mtx_client) != 0) {
        pthread_mutex_unlock(&client->mtx_client);
        //printf("out of release_client\n");
    }
    else {
        log_line("Tried to release non-locked client", LOG_WARN);
    }
}

client_t* get_client_by_index(int index) {
	//printf("Client.c: get_client_by_index\n");
	if(index>= 0 && MAX_CURRENT_CLIENTS > index && clients[index]) {    
		pthread_mutex_lock(&clients[index]->mtx_client);

		return clients[index];
	}

	return NULL;
}

void update_client_timestamp(client_t *client) {
	//printf("Client.c: update_client_timestamp\n");
	if(client != NULL) {
		gettimeofday(&client->timestamp, NULL);
	}
}

void set_client_color(client_t *client, int color){
	if(color==COLOR_WHITE){
		client->color=COLOR_WHITE;
	}else if(color==COLOR_BLACK){
		client->color=COLOR_BLACK;
	}else{
		client->color=0;
	}
}

void remove_client(client_t **client) {  
//printf("Client.c: remove_client\n");          
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
        //free((*client)->name);
        //printf("before name release\n");
        //free((*client)->name);

		client_num --;

        /* Release client */
		release_client((*client));

		clear_client_dgram_queue((*client));
		free((*client)->dgram_queue);
		free((*client));
	}

	*client = NULL;
}

void clear_client_dgram_queue(client_t *client) {
	packet_t *packet = queue_front(client->dgram_queue);

	while(packet) {
		queue_pop(client->dgram_queue, 0);

		if(packet->state) {
			free(packet->payload);
		}

		free(packet->msg);
		free(packet);

		packet = queue_front(client->dgram_queue);
	}
}

void clear_all_clients() {
	//printf("Client.c: clear_all_clients\n");
	int i = 0;
	client_t *client;

	for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
		client = get_client_by_index(i);

		if(client) {
			remove_client(&client);
		}
	}
}

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


 void send_client_code(client_t *client) {
 	//printf("Client.c send_reconnect_code\n");
 	char *buff = (char *) malloc(11 + CODE_LEN);

 	sprintf(buff,
 		"CLIENT_CODE;%s;",
 		client->code
 		);
 	// printf("reconnect_code\n");
 	enqueue_dgram(client, buff, 1);

 	free(buff);
 }

 // void send_client_name(client_t *client){
 // 	char *buff = (char *) malloc(11 + NAME_LEN);

 // 	sprintf(buff,
 // 		"CLIENT_NAME;%s;",
 // 		client->name
 // 		);
 // 	// printf("reconnect_code\n");
 // 	enqueue_dgram(client, buff, 1);

 // 	free(buff);
 // }

 int get_client_index_by_code(char *code) {
    int i;
    
    for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
        if(client_code[i] && strncmp(client_code[i], code, CODE_LEN) == 0) {
            return i;
        }
    }
    
    return -1;
}
