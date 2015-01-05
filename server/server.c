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

#include "err.h"
#include "server.h"
#include "global.h"
#include "client.h"
#include "com.h"
#include "game.h"
#include "logger.h"




int server_sockfd;

struct timeval ts_start;

char log_buffer[LOG_BUFFER_SIZE];



void init_server(char *bind_ip, int port){
	int server_len, client_len;
	int n;
	struct sockaddr_in server_addr;
	//char dgram[512];

	server_sockfd =socket(AF_INET,SOCK_DGRAM,0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(bind_ip);
	server_addr.sin_port = htons(port);

	server_len = sizeof(server_addr);
	//client_len = sizeof(client_len);

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

// void read_received_mgs(char *dgram, struct sockaddr_in *addr){
// 	char *msg;
// 	msg = strtok(dgram,";");
// 	client_t *client;
// 	char addr_str[INET_ADDRSTRLEN];
// 	inet_ntop(AF_INET, &addr->sin_addr, addr_str,INET_ADDRSTRLEN);

// 	if (strncmp(msg,"CONNECT",10)==0)
// 	{
// 		sprintf(log_buffer,"Call connect");
// 		add_client(addr);
// 		client=get_client_by_addr(addr);
// 		if (client)
// 		{
// 			/* code */
// 			release_client(client);
// 		}
// 		//printf("server.c: client found\n");

// 	}else if (strncmp(msg,"CREATE_GAME",11)==0){
// 		printf("Create game\n");
// 		client=get_client_by_addr(addr);
// 		if (client!=NULL)
// 		{
// 			create_game(client);
// 			/* code */
// 		}else
// 		{
// 			printf("cant create game: client==null\n");
// 		}


// 	}else if (strncmp(msg,"JOIN_GAME",9)==0){
// 		printf("Join game\n");
// 	}


// }

// void process_dgram(char *dgram, struct sockaddr_in *addr){
// 	char *token;
// 	int token_len;
// 	char *type;
// 	char *generic_chbuff;
// 	int packet_seq_id;

// 	client_t *client;

// 	unsigned int generic_uint;

// 	char addr_str[INET_ADDRSTRLEN];

// 	inet_ntop(AF_INET, &addr->sin_addr, addr_str,INET_ADDRSTRLEN);

// 	 sprintf(log_buffer,
//             "DATA_IN: %s <--- %s:%d",
//             dgram,
// 	    addr_str,
// 	    htons(addr->sin_port)
//             );
//     log_line(log_buffer, LOG_ALWAYS);

//     token = strtok(dgram, ";");
//     token_len = strlen(token);

//     if (strncmp(token,"CONNECT",10)==0)
// 	{
// 		printf("Call: Add client\n");
// 		add_client(addr);
// 		client=get_client_by_addr(addr);
// 		if (client)
// 		{
// 			/* code */
// 			release_client(client);
// 		}
// 		//printf("server.c: client found\n");

// 	}else if (strncmp(token,"CREATE_GAME",11)==0){
// 		printf("Create game\n");
// 		client=get_client_by_addr(addr);
// 		if (client!=NULL)
// 		{
// 			create_game(client);
// 			/* code */
// 		}else
// 		{
// 			printf("cant create game: client==null\n");
// 		}


// 	}else if (strncmp(token,"JOIN_GAME",9)==0){
// 		printf("Join game\n");
// 	}

void process_dgram(char *dgram, struct sockaddr_in *addr) {
	//printf("process_dgram\n");
    /* Protocol token */
    char *token;
    /* Token length */
    int token_len;
    /* Command */
    char *type;
    /* Generic char buffer */
    char *generic_chbuff;
    /* Sequential ID of received packet */
    int packet_seq_id;
    /* Client which we receive from */
    client_t *client;
    /* Generic unsigned int var */
    unsigned int generic_uint;
    /* String representation of address */
    char addr_str[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &addr->sin_addr, addr_str, INET_ADDRSTRLEN);
    
    /* Log */

    sprintf(log_buffer,
        "DATA_IN: %s <--- %s:%d",
        dgram,
        addr_str,
        htons(addr->sin_port)
        );
    log_line(log_buffer, LOG_DEBUG);
    //printf("log process_dgram\n");
    
    token = strtok(dgram, ";");
    token_len = strlen(token);
    //printf("%s\n",token );
    
    generic_chbuff = strtok(NULL, ";");
    packet_seq_id = (int) strtol(generic_chbuff, NULL, 0);
    //printf("%s\n",generic_chbuff );
    
    /* Check if datagram belongs to us */
    if( (token_len == strlen(STRINGIFY(APP_TOKEN))) &&
        strncmp(token, STRINGIFY(APP_TOKEN), strlen(STRINGIFY(APP_TOKEN))) == 0 &&
        packet_seq_id > 0) {
        type = strtok(NULL, ";");
    //printf("%s\n",type );


        /* New client connection */
    if(strncmp(type, "CONNECT", 7) == 0) {

        add_client(addr);            
        client = get_client_by_addr(addr);
        //printf("Queue Size: %d\n",queue_size(client->dgram_queue));

        if(client) {
            send_ack(client, 1, 0);
            send_reconnect_code(client);

                /* Release client */
            release_client(client);
        }

    }
        /* Reconnect */
        //else if(strncmp(type, "RECONNECT", 9) == 0) {            
            // client = get_client_by_index(get_client_index_by_rcode(strtok(NULL, ";")));

            // if(client) {
            //     // Sends ACK aswell after resetting clients SEQ_ID 
            //     reconnect_client(client, addr);

            //     /* Release client */
            //     release_client(client);
            // }
        //}
        /* Client should already exist */
    else {
        //printf("251\n");
        client = get_client_by_addr(addr);
        //printf("Queue Size: %d\n",queue_size(client->dgram_queue));
        if (client!=NULL)
        {
            printf("expected id is: %d\n",client->pkt_recv_seq_id );
            if (packet_seq_id == client->pkt_recv_seq_id)
            {
                /* code */

                printf("client!=NULL\n");
                if (strncmp(type,"CREATE_GAME", 11)==0)
                {
                    send_ack(client,packet_seq_id,0);
                    create_game(client);
                    /* code */
                }else if(strncmp(type,"JOIN_GAME", 9)==0){
                    printf("join_game accepted\n");

                    //send_ack(client);
                    send_ack(client, packet_seq_id, 0);
                    //char* code;
                    //game_t *tmp_game;
                    //tmp_game = get_game_by_index(0);
                    //code = tmp_game->code;
                    //release_game(tmp_game);
                    //printf("%s\n",code );
                    join_game(client,strtok(NULL, ";"));


                } else if(strncmp(type, "ACK", 3) == 0) {
                    //printf("receive ack\n");

                    recv_ack(client, 
                        (int) strtoul(strtok(NULL, ";"), NULL, 10));

                    update_client_timestamp(client);

                } else if (strncmp(type,"BTN1_PUSHED",11)==0){
                    printf("BTN1_PUSHED\n");
                    button_pushed(client,1);
                    send_ack(client,packet_seq_id,0);

                }else if (strncmp(type,"BTN2_PUSHED",11)==0){
                    printf("BTN2_PUSHED\n");
                    button_pushed(client,2);
                    send_ack(client,packet_seq_id,0);

                }else if(strncmp(type,"LEAVE_GAME",10)==0){
                    send_ack(client,packet_seq_id,0);
                    leave_game(client);

                }else if (strncmp(type,"START_GAME",10)==0){
                    send_ack(client,packet_seq_id,0);
                    start_game(client);
                }else if(strncmp(type,"SHOW_GAMES",10)==0){
                    send_ack(client,packet_seq_id,0);
                    show_games(client);
                }else if (strncmp(type,"OTHER_PLAYER_MOVE",17)==0){
                    send_ack(client,packet_seq_id,0);
                    //ask_other_to_move(client);
                }else if (strncmp(type,"MOVE",4)==0){
                    send_ack(client,packet_seq_id,0);
                    //generic_chbuff = strtok(NULL,";");
                    //unsigned int startRow = (unsigned int ) strtoul(generic_chbuff,NULL,10);
                    //generic_chbuff = strtok(NULL,";");
                    //unsigned int startCol = (unsigned int ) strtoul(generic_chbuff,NULL,10);
                    //generic_chbuff = strtok(NULL,";");
                    //unsigned int targetRow = (unsigned int ) strtoul(generic_chbuff,NULL,10);
                    //generic_chbuff = strtok(NULL,";");
                    //unsigned int targetCol = (unsigned int ) strtoul(generic_chbuff,NULL,10);
                    

                    send_figure_moved(client, (int) strtoul(strtok(NULL, ";"), NULL, 10),(int) strtoul(strtok(NULL, ";"), NULL, 10),(int) strtoul(strtok(NULL, ";"), NULL, 10),(int) strtoul(strtok(NULL, ";"), NULL, 10));
                    

                }

            }else if(packet_seq_id < client->pkt_recv_seq_id &&
                strncmp(type, "ACK", 3) != 0) {
                printf("Resend ack\n");
                send_ack(client, packet_seq_id, 1);

            }
            //printf("273\n");
                /* code */
        }
        //printf("276\n");
        if(client != NULL) {

                    /* Release client */
            release_client(client);
        }
    }

    //printf("278\n");
}
//printf("280\n");
}

        //         /* Check if expected seq ID matches */
        //         if(packet_seq_id == client->pkt_recv_seq_id) {

        //             /* Get command */
        //             if(strncmp(type, "CREATE_GAME", 11) == 0) {

        //                 // /* ACK client */
        //                 // send_ack(client, packet_seq_id, 0);

        //                 // create_game(client);

        //             }
        //             /* Receive ACK packet */
        //             else if(strncmp(type, "ACK", 3) == 0) {

        //                 // recv_ack(client, 
        //                 //         (int) strtoul(strtok(NULL, ";"), NULL, 10));

        //                 // update_client_timestamp(client);

        //             }
        //             /* Close client connection */
        //             else if(strncmp(type, "CLOSE", 5) == 0) {

        //                 /* ACK client */
        //                 // send_ack(client, packet_seq_id, 0);

        //                 // leave_game(client);
        //                 // remove_client(&client);

        //             }
        //             /* Keepalive loop */
        //             else if(strncmp(type, "KEEPALIVE", 9) == 0) {

        //                 /* ACK client */
        //                 // send_ack(client, packet_seq_id, 0);

        //             }
        //             /* Join existing game */
        //             else if(strncmp(type, "JOIN_GAME", 9) == 0) {

        //                 // /* ACK client */
        //                 // send_ack(client, packet_seq_id, 0);

        //                 // join_game(client, strtok(NULL, ";"));

        //             }
        //              Leave existing game 
        //             else if(strncmp(type, "LEAVE_GAME", 10) == 0) {

        //                 // /* ACK client */
        //                 // send_ack(client, packet_seq_id, 0);

        //                 // leave_game(client);
        //             }
        //             /* Start game */
        //             else if(strncmp(type, "START_GAME", 10) == 0) {

        //                 // /* ACK client */
        //                 // send_ack(client, packet_seq_id, 0);

        //                 // start_game(client);
        //             }
        //             /* Rolling die */
        //             else if(strncmp(type, "DIE_ROLL", 8) == 0) {

        //                 // /* ACK client */
        //                 // send_ack(client, packet_seq_id, 0);

        //                 // roll_die(client);
        //             }
        //             /* Moving figure */
        //             else if(strncmp(type, "FIGURE_MOVE", 11) == 0) {

        //                 /* ACK client */
        //                 // send_ack(client, packet_seq_id, 0);

        //                 // /* Parse figure id */
        //                 // generic_chbuff = strtok(NULL, ";");
        //                 // generic_uint = (unsigned int) strtoul(generic_chbuff, NULL, 10);

        //                 // move_figure(client, generic_uint);
        //             }

        //         }
        //         /* Packet was already processed */
        //         else if(packet_seq_id < client->pkt_recv_seq_id &&
        //             //     strncmp(type, "ACK", 3) != 0) {

        //             // send_ack(client, packet_seq_id, 1);

        //         }

        //         /* If client didnt close conection */
        //         if(client != NULL) {

        //             /* Release client */
        //             release_client(client);
        //         }
            //}
         //}
    //}










void set_socket_timeout_linux() {
	struct timeval cur_tv;

	cur_tv.tv_sec = 1;
	cur_tv.tv_usec = 0;

	if(setsockopt(server_sockfd, SOL_SOCKET, SO_RCVTIMEO, &cur_tv, sizeof(cur_tv)) < 0) {
		raise_error("Error setting socket timeout (linux).");
	}
}



