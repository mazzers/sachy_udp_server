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

#include "server.h"
#include "client.h"
#include "game.h"
#include "global.h"
#include "err.h"
#include "logger.h"




int server_sockfd,client_sockfd;

struct timeval ts_start;

char log_buffer[LOG_BUFFER_SIZE];



void init_server(char *bind_ip, int port){
	int server_len, client_len;
	int n;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	char dgram[512];

	server_sockfd =socket(AF_INET,SOCK_DGRAM,0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(bind_ip);
	server_addr.sin_port = htons(port);

	server_len = sizeof(server_addr);
	client_len = sizeof(client_len);

	if (bind(server_sockfd,(struct sockaddr *) &server_addr, server_len)!=0){
		raise_error("Error binding, exiting.");

	}

	    set_socket_timeout_linux();
    
    /* Log */
    sprintf(log_buffer,
            "Starting server with IP %s and port %d",
            bind_ip,
            port
            );
    
    log_line(log_buffer, LOG_ALWAYS);

	// while(1){
	// 	printf("Server ceka\n");
	// 	n = recvfrom(server_sockfd,&dgram,sizeof(dgram),0,(struct sockaddr *) &client_addr, &client_len);
	// 	printf("Got data\n%s\n",dgram );
	// 	read_received_mgs(dgram, &client_addr);







	// }




}

void read_received_mgs(char *dgram, struct sockaddr_in *addr){
	char *msg;
	msg = strtok(dgram,";");
	client_t *client;
	char addr_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &addr->sin_addr, addr_str,INET_ADDRSTRLEN);

	if (strncmp(msg,"CONNECT",10)==0)
	{
		printf("Call: Add client\n");
		add_client(addr);
		client=get_client_by_addr(addr);
		if (client)
		{
			/* code */
			release_client(client);
		}
		//printf("server.c: client found\n");

	}else if (strncmp(msg,"CREATE_GAME",11)==0){
		printf("Create game\n");
		client=get_client_by_addr(addr);
		if (client!=NULL)
		{
			create_game(client);
			/* code */
		}else
		{
			printf("cant create game: client==null\n");
		}
		

	}else if (strncmp(msg,"JOIN_GAME",9)==0){
		printf("Join game\n");
	}


}

void process_dgram(char *dgram, struct sockaddr_in *addr){
	char *token;
	int token_len;
	char *type;
	char *generic_chbuff;
	int packet_seq_id;

	client_t *client;

	unsigned int generic_uint;

	char addr_str[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &addr->sin_addr, addr_str,INET_ADDRSTRLEN);

	 sprintf(log_buffer,
            "DATA_IN: %s <--- %s:%d",
            dgram,
	    addr_str,
	    htons(addr->sin_port)
            );
    log_line(log_buffer, LOG_ALWAYS);

    token = strtok(dgram, ";");
    token_len = strlen(token);

    if (strncmp(token,"CONNECT",10)==0)
	{
		printf("Call: Add client\n");
		add_client(addr);
		client=get_client_by_addr(addr);
		if (client)
		{
			/* code */
			release_client(client);
		}
		//printf("server.c: client found\n");

	}else if (strncmp(token,"CREATE_GAME",11)==0){
		printf("Create game\n");
		client=get_client_by_addr(addr);
		if (client!=NULL)
		{
			create_game(client);
			/* code */
		}else
		{
			printf("cant create game: client==null\n");
		}
		

	}else if (strncmp(token,"JOIN_GAME",9)==0){
		printf("Join game\n");
	}








}

void set_socket_timeout_linux() {
    struct timeval cur_tv;
    
    cur_tv.tv_sec = 1;
    cur_tv.tv_usec = 0;
    
    if(setsockopt(server_sockfd, SOL_SOCKET, SO_RCVTIMEO, &cur_tv, sizeof(cur_tv)) < 0) {
        raise_error("Error setting socket timeout (linux).");
    }
}



