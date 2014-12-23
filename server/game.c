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

game_t *games[MAX_CURRENT_GAMES];

void create_game(client_t *client){
	printf("game.c: create_game\n");
	char *message;
	char buff[11];
	unsigned int message_len;
	printf("game.c: create_game\n");
	int i=0;

	if(client->game_index== -1){
		printf("game.c: client->game_index=-1\n");
		game_t *game = (game_t *) malloc(sizeof(game_t));
		gettimeofday(&game->timestamp,NULL);
		memset(game->player_index, -1, sizeof(int)*2);
		game->player_index[0] = client->client_index;
		game->state=0;
		game->player_num=1;

		pthread_mutex_init(&game->mtx_game,NULL);
		printf("game.c: b4 for\n");

		game->code = (char *) malloc(GAME_CODE_LEN + 1);    
		generate_game_code(game->code, 0);

		if(game->code[0] != 0){
			for(i=0;i<MAX_CURRENT_GAMES;i++){
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
			client->game_index = game->game_index;


		}

	}
}

game_t* get_game_by_code(char *code) {
	printf("game.c: get_game_by_code\n");
	int i;

	for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
		if(games[i]) {
			pthread_mutex_lock(&games[i]->mtx_game);

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
    // game_t *game;
    // int i, n;
    // int len;
    // char *buff;
    
    // if(client != NULL) {
    //     game = get_game_by_index(client->game_index);
        
    //     if(game) {
    //         /* Log */
    //         sprintf(log_buffer,
    //                 "Client with index %d is leaving game with code %s and index %d",
    //                 client->client_index,
    //                 game->code,
    //                 game->game_index
    //                 );
            
    //         log_line(log_buffer, LOG_DEBUG);
            
    //         if(game->player_num == 1) {
                
    //             remove_game(&game, client);
                
    //         }
    //         else {
                
    //             for(i = 0; i < 4; i++) {
    //                 /* If client index matches */
    //                 if(game->player_index[i] == client->client_index) {
    //                     game->player_index[i] = -1;
    //                     game->player_num--;
                        
    //                     /* If game is running */
    //                     if(game->state) {
    //                         /* Reset clients figures */
    //                         for(n = 4 * i; n < (4 * i) + 4; n++) {
    //                             /* Place figures at their starting position */
    //                             game->game_state.figures[n] = 56 + n;
    //                             game->game_state.fields[56 + n] = -1;
    //                         }

    //                         /* If leaving player was supposed to play next */
    //                         if(game->game_state.playing == i) {
    //                             /* Find next player that will be playing */
    //                             set_game_playing(game);
    //                         }
    //                     }
                        
    //                     break;
    //                 }
    //             }
                
    //             len = 30 + 11;
    //             buff = (char *) malloc(len);
                                
    //             /* @TODO: if he wasnt playing do something else */
    //             /* Notify other players that one left */
    //             sprintf(buff, 
    //                     "CLIENT_LEFT_GAME;%d;%d;%d", 
    //                     i, 
    //                     game->game_state.playing,
    //                     GAME_MAX_PLAY_TIME_SEC - 1
    //                     );
                
    //             broadcast_game(game, buff, NULL, 0);
                
    //             free(buff);
    //         }
            
    //         /* Reset client's game code */
    //         client->game_index = -1;
            
    //         enqueue_dgram(client, "GAME_LEFT", 1);
            
    //         if(game) {
    //             /* Release game */
    //             release_game(game);
    //         }
    //     }
    // }
}

int timeout_game(client_t *client) {
	printf("game.c: timeout_game\n");
    // int i;
    // char buff[40];
    // game_t *game = get_game_by_index(client->game_index);
    
    // if(game) {
    //     if(game->state) {
    //         /* Log */
    //         sprintf(log_buffer,
    //                 "Client with index %d timeouted from game with code %s and index %d, can reconnect",
    //                 client->client_index,
    //                 game->code,
    //                 game->game_index
    //                 );

    //         log_line(log_buffer, LOG_DEBUG);

    //         /* Wont wait for timeouted player if he is alone in game */
    //         if(game->player_num > 1) {
    //             for(i = 0; i < 4; i++) {
    //                 if(game->player_index[i] == client->client_index) {
    //                     break;
    //                 }
    //             }
                
    //             if(game->game_state.playing == i) {
    //                 set_game_playing(game);
    //             }
                
    //             sprintf(buff, 
    //                     "CLIENT_TIMEOUT;%d;%d;%d", 
    //                     i, 
    //                     game->game_state.playing,
    //                     GAME_MAX_PLAY_TIME_SEC - 1
    //                     );
                
    //             broadcast_game(game, buff, client, 0);
                                
    //             /* Update clients timestamp (starts countdown for max timeout time) */
    //             update_client_timestamp(client);

    //             /* Release game */
    //             release_game(game);
                
    //             return 1;
    //         }
    //         else {
    //             remove_game(&game, client);
    //         }
    //     }
        
    //     if(game) {
    //         /* Release game */
    //         release_game(game);
    //     }
    // }
    
    // return 0;
}