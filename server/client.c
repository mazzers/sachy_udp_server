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

//clients array
client_t *clients[MAX_CURRENT_CLIENTS]={NULL};

unsigned int client_num=0;

void add_client(struct sockaddr_in *addr){
	printf("Client.c: add_client\n");
	client_t *new_client;
	struct sockaddr_in *new_addr;
	int i;
	new_client = (client_t *) malloc(sizeof(client_t));

	new_addr = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));

	memcpy(new_addr, addr, sizeof(struct sockaddr_in));
	new_client->addr = new_addr;
	new_client->game_index=-1;
	for(i=0;i<MAX_CURRENT_CLIENTS;i++){
		if(clients[i]==NULL){
			new_client->client_index=i;
			clients[i]=new_client;
			printf("New user added with index%d\n",i);
			client_num++;
			printf("client.c: clients %d\n",client_num );
			break;
		}



	}






}
