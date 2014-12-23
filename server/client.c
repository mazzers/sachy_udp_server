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
#include "global.h"
#include "server.h"
#include "game.h"
#include "queue.h"
#include "com.h"
#include "logger.h"

//clients array
client_t *clients[MAX_CURRENT_CLIENTS]={NULL};

unsigned int client_num=0;

void add_client(struct sockaddr_in *addr){
	printf("Client.c: add_client\n");
	client_t *new_client;
	client_t *existing_client;
	struct sockaddr_in *new_addr;
	int i;

	if (client_num < MAX_CURRENT_CLIENTS)
	{
		printf("client_num < MAX_CURRENT_CLIENTS\n");
		existing_client = get_client_by_addr(addr);
		printf("existing_client = get_client_by_addr(addr)\n");
		if (existing_client==NULL)
		{

			printf("client.c: it is new client\n");
			new_client = (client_t *) malloc(sizeof(client_t));

			new_addr = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));

			memcpy(new_addr, addr, sizeof(struct sockaddr_in));
			new_client->addr = new_addr;
			char *some_addr;
			some_addr = inet_ntoa(new_client->addr->sin_addr);
			printf("client->addr%s\n",some_addr );
			//int port = inet_htons(new_client->addr->sin_port);
			//printf("client->addr%d\n",port);
			new_client->game_index=-1;
			printf("client->gameindex=%d\n",new_client->game_index );
			new_client->addr_str = malloc(INET_ADDRSTRLEN +1);
			 new_client->pkt_recv_seq_id = 1;
            new_client->pkt_send_seq_id = 1;
            new_client->state = 1;
            new_client->dgram_queue = malloc(sizeof(Queue));

			pthread_mutex_init(&new_client->mtx_client, NULL);
			inet_ntop(AF_INET, &addr->sin_addr, new_client->addr_str, INET_ADDRSTRLEN);
			for(i=0;i<MAX_CURRENT_CLIENTS;i++){
				if(clients[i]==NULL){
					new_client->client_index=i;
					clients[i]=new_client;
					printf("New user added with index %d\n",i);
					client_num++;
					printf("client.c: clients %d\n",client_num );
					break;
				}



			}
		}else{
			printf("client.c: client exists\n");
			release_client(existing_client);


		}
		
	}else{
		printf("New client tried to connec but server is full\n");
	}
}

// client_t* get_client_by_addr(struct sockaddr_in *addr){
// 	int i=0;
// 	char addr_str[INET_ADDRSTRLEN];
// 	inet_ntop(AF_INET, &(addr)->sin_addr, addr_str, INET_ADDRSTRLEN);

// 	for (i = 0; i < MAX_CURRENT_CLIENTS; i++)
// 	{
// 		if(clients[i]!=NULL){
// 			if(strncmp(clients[i]->addr_str, addr_str, INET_ADDRSTRLEN) == 0
// 				&& (htons((addr)->sin_port) == htons(clients[i]->addr->sin_port) )){
// 				printf("client.c: client found\n");
// 			return clients[i];
// 		}

// 	}
// }
// printf("client.c: client not found\n");
// return NULL;


// }

client_t* get_client_by_addr(struct sockaddr_in *addr) {
	printf("get_client_by_addr\n");
	int i = 0;
	char addr_str[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &(addr)->sin_addr, addr_str, INET_ADDRSTRLEN);

	for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
		printf("for: %d\n",i );
		if(clients[i] != NULL) {
			pthread_mutex_lock(&clients[i]->mtx_client);
			printf("pthread_mutex_lock\n");

            /* Check if client still exists */
			if(clients[i] != NULL) {
				printf("client[%d]!=NULL\n",i );
                    /* Check if address and port matches */
				if(strncmp(clients[i]->addr_str, addr_str, INET_ADDRSTRLEN) == 0
					&& (htons((addr)->sin_port) == htons(clients[i]->addr->sin_port) )) {
					printf("client matched\n");
				return clients[i];

			}else{
				printf("client not matched\n");
			}
		}
		printf("call release_client\n");
		release_client(clients[i]);
	}
}

return NULL;
}

void release_client(client_t *client) {  
	printf("release_client\n");  
	if(client && pthread_mutex_trylock(&client->mtx_client) != 0) {
		pthread_mutex_unlock(&client->mtx_client);
		printf("out of release_client\n");
	}
	else {
        //log_line("Tried to release non-locked client", LOG_WARN);
		printf("Tried to release non-locked client\n");
	}
}

client_t* get_client_by_index(int index) {
	if(index>= 0 && MAX_CURRENT_CLIENTS > index && clients[index]) {    
		pthread_mutex_lock(&clients[index]->mtx_client);

		return clients[index];
	}

	return NULL;
}

void update_client_timestamp(client_t *client) {
    if(client != NULL) {
        gettimeofday(&client->timestamp, NULL);
    }
}