#ifndef GAME_H
#define GAME_H

#include "client.h"
extern unsigned int game_num;

typedef struct{
	int player_indexes[2];
	int game_index;
	int player_num;

		




}game_t;

void create_game(client_t *client);

#endif