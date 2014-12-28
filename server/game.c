#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "client.h"
#include "global.h"
#include "logger.h"
#include "com.h"

unsigned int game_num=0;
char log_buffer[LOG_BUFFER_SIZE];

game_t *games[MAX_CURRENT_CLIENTS];

void create_game(client_t *client){
	printf("game.c: create_game\n");
	char *message;
	char buff[11];
	unsigned int message_len;
	int i=0;

	if(client->game_index== -1){
		game_t *game = (game_t *) malloc(sizeof(game_t));
		gettimeofday(&game->timestamp,NULL);
		game->state=0;
		game->player_num=1;
		pthread_mutex_init(&game->mtx_game,NULL);

		memset(game->player_index, -1, sizeof(int)*2);
		game->player_index[0] = client->client_index;
		
		printf("game.c mutex\n");
		
		printf("here 0\n");
		game->code = (char *) malloc(GAME_CODE_LEN + 1);    
		generate_game_code(game->code, 0);
		printf("here\n");

		if(game->code[0] != 0){
			for(i=0;i<MAX_CURRENT_CLIENTS;i++){
				if(games[i]==NULL){
					games[i]=game;
					game->game_index=i;
					printf("game.c: add game with index %d\n",i );
					break;
				}


			}
			game_num++;
			printf("game.c: games: %d\n", game_num);

			sprintf(buff, "%d", ( GAME_MAX_LOBBY_TIME_SEC - 1));
			message_len = strlen(buff) + GAME_CODE_LEN + 14 + 1;
			message = (char *) malloc(message_len);

			sprintf(message, "GAME_CREATED;%s;%d", game->code, GAME_MAX_LOBBY_TIME_SEC - 1);

			enqueue_dgram(client, message, 1);
			
			sprintf(log_buffer,
				"Created new game with code %s and index %d",
				game->code,
				game->game_index
				);
			log_line(log_buffer, LOG_DEBUG);
			client->game_index = game->game_index;
			free(message);


		}

	}
}

game_t* get_game_by_code(char *code) {
	printf("game.c: get_game_by_code\n");
	int i;

	for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
		//printf("%d\n",i );
		if(games[i]) {
			printf("it's game with index: %d\n",i );
			pthread_mutex_lock(&games[i]->mtx_game);
			printf("mutex lock get get_game_by_code\n");

			if(strncmp(code, games[i]->code, GAME_CODE_LEN) == 0) {
				return games[i];
			}

			pthread_mutex_unlock(&games[i]->mtx_game);
		}
	}

	return NULL;
}

void generate_game_code(char *code, unsigned int iteration) {
	printf("game.c: generate_game_code\n");
	game_t* existing_game;

    /* Prevent infinite loop */
	if(iteration > 100) {
		code[0] = 0;

		return;
	}

	gen_random(code, GAME_CODE_LEN);
	existing_game = get_game_by_code(code);

	if(existing_game) {
		release_game(existing_game);
		generate_game_code(code, iteration++);
	}
}

void remove_game(game_t **game, client_t *skip) {
	printf("game.c remove_game\n");
	int i;
	client_t *cur;

	if(game != NULL) {
        /* Log */
		sprintf(log_buffer,
			"Removing game with code %s and index %d",
			(*game)->code,
			(*game)->game_index
			);

		log_line(log_buffer, LOG_DEBUG);

        /* Set all player's game index to - 1 */
		for(i = 0; i < 4; i++) {
			if((*game)->player_index[i] != -1 && 
				(!skip || (*game)->player_index[i] != skip->client_index)) {

				cur = get_client_by_index((*game)->player_index[i]);

			if(cur) {
				cur->game_index = -1;

                    /* Release client */
				release_client(cur);
			}
		}
	}

        //free((*game)->code);

	games[(*game)->game_index] = NULL;
	game_num--;

	release_game((*game));

	free((*game));
}

*game = NULL;
}

void broadcast_game(game_t *game, char *msg, client_t *skip, int send_skip) {
	printf("game.c: broadcast_game\n");
	int i;
	client_t *client;
	int release;

	if(game != NULL) {        
		for(i = 0; i < 2; i++) {
			client = NULL;
			release = 1;

            /* Player exists */
			if(game->player_index[i] != -1) {                
                /* If we want to skip client */
				if(!skip || game->player_index[i] != skip->client_index) {
					client = get_client_by_index(game->player_index[i]);
				}
				else if(skip && send_skip) {
					client = skip;
					release = 0;
				}

				if(client != NULL) {
					if(client->state) {
						enqueue_dgram(client, msg, 1);
					}

					if(release) {
                        /* Release client */
						release_client(client);
					}
				}

			}
		}
	}
}

void clear_all_games() {
	printf("game.c: clear_all_games\n");
	int i = 0;
	game_t *game;

	for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
		game = get_game_by_index(i);

		if(game) {
			games[i] = NULL;
			free(game->code);

			pthread_mutex_unlock(&game->mtx_game);
			free(game);
		}
	}
}

game_t* get_game_by_index(unsigned int index) {
	printf("game.c: get_game_by_index\n");
	if(index>= 0 && MAX_CURRENT_CLIENTS > index) {
		if(games[index]) {
			pthread_mutex_lock(&games[index]->mtx_game);

			return games[index];
		}
	}

	return NULL;
}

void release_game(game_t *game) {
	printf("game.c: release_game\n");
	if(game && pthread_mutex_trylock(&game->mtx_game) != 0) {
		pthread_mutex_unlock(&game->mtx_game);
	}
	else {
		log_line("Tried to release non-locked game", LOG_WARN);
	}
}

void leave_game(client_t *client) {
	printf("game.c: leave_game\n");
	game_t *game;
	int i, n;
	int len;
	char *buff;

	if(client != NULL) {
		game = get_game_by_index(client->game_index);

		if(game) {
            /* Log */
			sprintf(log_buffer,
				"Client with index %d is leaving game with code %s and index %d",
				client->client_index,
				game->code,
				game->game_index
				);

			log_line(log_buffer, LOG_DEBUG);

			if(game->player_num == 1) {

				remove_game(&game, client);

			}
			else {

				for(i = 0; i < 2; i++) {
                    /* If client index matches */
					if(game->player_index[i] == client->client_index) {
						game->player_index[i] = -1;
						game->player_num--;

                        /* If game is running */
						if(game->state) {
							printf("game.c missing mechanics\n");
                            // /* Reset clients figures */
                            // for(n = 2 * i; n < (2 * i) + 2; n++) {
                            //     /* Place figures at their starting position */
                            //     game->game_state.figures[n] = 56 + n;
                            //     game->game_state.fields[56 + n] = -1;
                            // }

                            // /* If leaving player was supposed to play next */
                            // if(game->game_state.playing == i) {
                            //     /* Find next player that will be playing */
                            //     set_game_playing(game);
                            // }
						}

						break;
					}
				}

				len = 30 + 11;
                //buff = (char *) malloc(len);
				printf("here something\n");
                /* @TODO: if he wasnt playing do something else */
                /* Notify other players that one left */
                // sprintf(buff, 
                //         "CLIENT_LEFT_GAME;%d;%d;%d", 
                //         i, 
                //         game->game_state.playing,
                //         GAME_MAX_PLAY_TIME_SEC - 1
                //         );

                // broadcast_game(game, buff, NULL, 0);

                //free(buff);
			}

            /* Reset client's game code */
			client->game_index = -1;

			enqueue_dgram(client, "GAME_LEFT", 1);

			if(game) {
                /* Release game */
				release_game(game);
			}
		}
	}
}

int timeout_game(client_t *client) {
	printf("game.c: timeout_game\n");
	int i;
	char buff[40];
	game_t *game = get_game_by_index(client->game_index);

	if(game) {
		if(game->state) {
            /* Log */
			sprintf(log_buffer,
				"Client with index %d timeouted from game with code %s and index %d, can reconnect",
				client->client_index,
				game->code,
				game->game_index
				);

			log_line(log_buffer, LOG_DEBUG);

            /* Wont wait for timeouted player if he is alone in game */
			if(game->player_num > 1) {
				for(i = 0; i < 2; i++) {
					if(game->player_index[i] == client->client_index) {
						break;
					}
				}

                // if(game->game_state.playing == i) {
                //     set_game_playing(game);
                // }

                // sprintf(buff, 
                //         "CLIENT_TIMEOUT;%d;%d;%d", 
                //         i, 
                //         game->game_state.playing,
                //         GAME_MAX_PLAY_TIME_SEC - 1
                //         );

                // broadcast_game(game, buff, client, 0);

                /* Update clients timestamp (starts countdown for max timeout time) */
				update_client_timestamp(client);

                /* Release game */
				release_game(game);

				return 1;
			}
			else {
				remove_game(&game, client);
			}
		}

		if(game) {
            /* Release game */
			release_game(game);
		}
	}

	return 0;
}

void join_game(client_t *client, char* game_code) {
	printf("game.c: join_game\n");
	int i;
	game_t *game;
	char buff[21];

    /* Check if client is already in a game */
	if(client->game_index == -1) {
		game = get_game_by_code(game_code);
	}
	printf("397\n");
	if(game) {
		printf("399\n");
		if(!game->state) {
			printf("400\n");
            /* If we have a spot */
			if(game->player_num < 2) {

                /* Find spot for player */
				for(i = 0; i < 2; i++) {
					if(game->player_index[i] == -1) {
						game->player_index[i] = client->client_index;

						break;
					}
				}

                /* Prepare message */
				strcpy(buff, "CLIENT_JOINED_GAME;");
				buff[19] = (char) (((int) '0') + i);
				buff[20] = 0;

                /* Set clients game index reference to this game */
				client->game_index = game->game_index;

                /* Send game state to joined client */
				send_game_state(client, game);

                /* Broadcast game, skipping current client */
				broadcast_game(game, buff, client, 1);

				game->player_num++;

                /* Log */
				sprintf(log_buffer,
					"Player with index %d joined game with code %s and index %d",
					client->client_index,
					game->code,
					game->game_index
					);

				log_line(log_buffer, LOG_DEBUG);

			}
            /* Game is full */
			else {
                /* Log */
				sprintf(log_buffer,
					"Client with index %d tried to join game with code %s and index %d, but game was full",
					client->client_index,
					game->code,
					game->game_index
					);

				log_line(log_buffer, LOG_DEBUG);

				enqueue_dgram(client, "GAME_FULL", 1);
			}
		}
        /* Game is already running */
		else {
            /* Log */
			sprintf(log_buffer,
				"Client with index %d tried to join game with code %s and index %d, but game was already running",
				client->client_index,
				game->code,
				game->game_index
				);

			log_line(log_buffer, LOG_DEBUG);

			enqueue_dgram(client, "GAME_RUNNING", 1);
		}

        /* Release game */
		release_game(game);
	}
    /* Non existent game, inform user */
	else {
        /* Log */
		sprintf(log_buffer,
			"Client with index %d tried to join game with code %s, but game DOESNT EXIST",
			client->client_index,
			game_code
			);

		log_line(log_buffer, LOG_DEBUG);

		enqueue_dgram(client, "GAME_NONEXISTENT", 1);
	}
}
void send_game_state(client_t *client, game_t *game) {
	printf("game.c send_game_state\n");
	char *buff;
	unsigned short release = 0;
	unsigned short player[2] = {0};
	unsigned int i;
	int client_game_index;
	client_t *cur_client;

	if(client != NULL) {
		if(game == NULL) {
			game = get_game_by_index(client->game_index);

			release = 1;
		}

		if(game) {
            /* If client is in that game */
			if(game->game_index == client->game_index) {
                /* Buffer is set to maximum possible size, but the actual message
                 * is terminated by 0 so client can get the actual length
                 */
                 buff = (char *) malloc(105 + GAME_CODE_LEN + 11);

                /* Get players that are playing */
                 for(i = 0; i < 2; i++) {
                 	if(game->player_index[i] != -1) {

                 		if(game->player_index[i] == client->client_index) {
                 			client_game_index = i;

                 			player[i] = 1;
                 		}
                 		else {
                 			cur_client = get_client_by_index(game->player_index[i]);

                 			if(cur_client) {
                                /* Client is active */
                 				if(cur_client->state) {
                 					player[i] = 1;
                 				}
                                /* Client timeouted */
                 				else {
                 					player[i] = 2;
                 				}

                                /* Release client */
                 				release_client(cur_client);
                 			}
                 		}
                 	}
                 }

                /* game code, game state, 4x player connected, 16x figure position,
                 * index of currently playing client, game index of connecting player
                 * and timeout before next state change (lobby timeout, playing timeout)
                 */
                 sprintf(buff,
                 	"GAME_STATE;%s;%u;%u;%u;%u;%d",
                 	game->code,
                 	game->state, 
                 	player[0],
                 	player[1],
                 	client_game_index,
                 	game_time_before_timeout(game)
                 	);

                 enqueue_dgram(client, buff, 1);

                 free(buff);
             }

             if(release) {
                /* Release game */
             	release_game(game);
             }
         }
     }
 }
 int game_time_before_timeout(game_t *game) {
 	printf("game.c: game_time_before_timeout\n");
 	struct timeval cur_tv;    
 	gettimeofday(&cur_tv, NULL);

    /* Game running */
 	if(game->state) {
 		return (GAME_MAX_PLAY_TIME_SEC - (cur_tv.tv_sec - game->timestamp.tv_sec));
 	}
    /* Game in lobby */
 	else {
 		return (GAME_MAX_LOBBY_TIME_SEC - (cur_tv.tv_sec - game->timestamp.tv_sec));
 	}
 }

 void button_pushed(client_t *client, int button){
 	client_t *cur_client;
 	game_t *game;
 	char buff[13];
 	if (button==1)
 	{
 		strcpy(buff, "BTN1_PUSHED;");
 	}
 	else{
 		strcpy(buff, "BTN2_PUSHED;");
 	}
 	//buff[11] = (char) (((int) ';'));
 	buff[12] = 0;
 	game = get_game_by_index(client->game_index);
 	//send_btn_pushed(client,game);
 	broadcast_game(game, buff, client, 1);
 	release_game(game);


 }

 //void send_btn_pushed(client_t *client, game_t *game){
 	//char buff[13]


 //}
 