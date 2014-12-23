#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "client.h"
#include "global.h"
#include "logger.h"

unsigned int game_num=0;
char log_buffer[LOG_BUFFER_SIZE];

game_t *games[MAX_CURRENT_GAMES];

void create_game(client_t *client){
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

		//sprintf(message, "GAME_CREATED;%s;%d", game->code, GAME_MAX_LOBBY_TIME_SEC - 1);

		enqueue_dgram(client, message, 1);
		client->game_index = game->game_index;


	}


}

void remove_game(game_t **game, client_t *skip) {
	int i;
	client_t *cur;

	if(game != NULL) {
        /* Log */
        // sprintf(log_buffer,
        //         "Removing game with code %s and index %d",
        //         (*game)->code,
        //         (*game)->game_index
        //         );

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
	int i = 0;
	game_t *game;

	for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
		game = get_game_by_index(i);

		if(game) {
			games[i] = NULL;
            //free(game->code);

			pthread_mutex_unlock(&game->mtx_game);
			free(game);
		}
	}
}

game_t* get_game_by_index(unsigned int index) {
	if(index>= 0 && MAX_CURRENT_CLIENTS > index) {
		if(games[index]) {
			pthread_mutex_lock(&games[index]->mtx_game);

			return games[index];
		}
	}

	return NULL;
}

void release_game(game_t *game) {
	if(game && pthread_mutex_trylock(&game->mtx_game) != 0) {
		pthread_mutex_unlock(&game->mtx_game);
	}
	else {
		log_line("Tried to release non-locked game", LOG_WARN);
	}
}