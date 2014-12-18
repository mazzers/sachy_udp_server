#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "client.h"
#include "global.h"

unsigned int game_num=0;

game_t *games[MAX_CURRENT_GAMES];

void create_game(client_t *client){
	printf("game.c: create_game\n");
	int i=0;

	if(client->game_index== -1){
		printf("game.c: client->game_index=-1\n");
		game_t *game = (game_t *) malloc(sizeof(game_t));
		memset(game->player_indexes, -1, sizeof(int)*2);
		game->player_indexes[0] = client->client_index;
		game->player_num=1;
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


	}


}


