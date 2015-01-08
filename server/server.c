#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>

#include <errno.h>

#include "logger.h"

#include "game.h"
#include "server.h"
#include "global.h"
#include "client.h"
#include "com.h"




/*Server socket*/
int server_sockfd;
/*Server start time*/
struct timeval ts_start;
/*Log buffer*/
char log_buffer[LOG_BUFFER_SIZE];


/*
___________________________________________________________

    Server initiation by client input. 
___________________________________________________________
*/
void init_server(char *bind_ip, int port){
	int server_len, client_len;
	int n;
	struct sockaddr_in server_addr;

	server_sockfd =socket(AF_INET,SOCK_DGRAM,0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(bind_ip);
	server_addr.sin_port = htons(port);

	server_len = sizeof(server_addr);

	if (bind(server_sockfd,(struct sockaddr *) &server_addr, server_len)!=0){
		raise_error("Error binding, shutdown server.");

	}

	set_socket_timeout_linux();


	sprintf(log_buffer,
		"Starting server with IP %s at port %d",
		bind_ip,
		port
		);

	log_line(log_buffer, LOG_ALWAYS);





}
/*
___________________________________________________________

    Process incoming datagrams. Check whether datagram token
    match to server token. Call appropriate method.
___________________________________________________________
*/

void process_dgram(char *dgram, struct sockaddr_in *addr) {
    /*Protokol token*/
    char *token;
    /*Token lenght*/
    int token_len;
    /*Incoming message*/
    char *type;
    /*Buffer*/
    char *generic_chbuff;
    /*Incoming ID*/
    int packet_seq_id;
    client_t *client;
    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, addr_str, INET_ADDRSTRLEN);

    sprintf(log_buffer,
        "DATA_IN: %s <--- %s:%d",
        dgram,
        addr_str,
        htons(addr->sin_port)
        );
    log_line(log_buffer, LOG_DEBUG);
    
    token = strtok(dgram, ";");
    token_len = strlen(token);

    generic_chbuff = strtok(NULL, ";");
    packet_seq_id = (int) strtol(generic_chbuff, NULL, 0);

    if( (token_len == strlen(STRINGIFY(TOKEN))) &&
        strncmp(token, STRINGIFY(TOKEN), strlen(STRINGIFY(TOKEN))) == 0 &&
        packet_seq_id > 0) {
        type = strtok(NULL, ";");

    if(strncmp(type, "CONNECT", 7) == 0) {

        add_client(addr);

        client = get_client_by_addr(addr);

        if(client) {
            send_ack(client, 1, 0);
            send_client_code(client);
            release_client(client);
        }
    }
    else {
        client = get_client_by_addr(addr);
        if (client!=NULL)
        {
            /*Check incoming ID*/
            if (packet_seq_id == client->pkt_recv_seq_id)
            {
                if (strncmp(type,"CREATE_GAME", 11)==0)
                {
                    send_ack(client,packet_seq_id,0);
                    create_game(client);
                }else if(strncmp(type,"JOIN_GAME", 9)==0){

                    send_ack(client, packet_seq_id, 0);
                    join_game(client,strtok(NULL, ";"));
                } else if(strncmp(type, "ACK", 3) == 0) {
                    recv_ack(client, 
                        (int) strtoul(strtok(NULL, ";"), NULL, 10));
                    update_client_timestamp(client);
                }else if(strncmp(type,"LEAVE_GAME",10)==0){
                    send_ack(client,packet_seq_id,0);
                    leave_game(client);
                }else if (strncmp(type,"START_GAME",10)==0){
                    send_ack(client,packet_seq_id,0);
                    start_game(client);
                }else if(strncmp(type,"SHOW_GAMES",10)==0){
                    send_ack(client,packet_seq_id,0);
                    show_games(client);
                }else if (strncmp(type,"MOVE",4)==0){
                    send_ack(client,packet_seq_id,0);
                    send_figure_moved(client, (int) strtoul(strtok(NULL, ";"), NULL, 10),
                        (int) strtoul(strtok(NULL, ";"), NULL, 10),
                        (int) strtoul(strtok(NULL, ";"), NULL, 10),
                        (int) strtoul(strtok(NULL, ";"), NULL, 10));
                }else if (strncmp(type,"KEEPALIVE",4)==0)
                {
                   send_ack(client, packet_seq_id, 0);
               }else if(strncmp(type,"IM",2)==0){
                send_ack(client, packet_seq_id, 0);
                set_client_color(client,(int) strtoul(strtok(NULL, ";"), NULL, 10));
            }
            /*Resend ack*/
        }else if(packet_seq_id < client->pkt_recv_seq_id &&
            strncmp(type, "ACK", 3) != 0) {
            send_ack(client, packet_seq_id, 1);
        }
    }
    if(client != NULL) {
        release_client(client);
    }
}
}
}
/*
___________________________________________________________

    Linux socket timeout.
___________________________________________________________
*/
void set_socket_timeout_linux() {
	struct timeval cur_tv;
	cur_tv.tv_sec = 1;
	cur_tv.tv_usec = 0;
	if(setsockopt(server_sockfd, SOL_SOCKET, SO_RCVTIMEO, &cur_tv, sizeof(cur_tv)) < 0) {
		raise_error("Error setting socket timeout.");
	}
}



