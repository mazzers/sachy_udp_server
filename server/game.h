#ifndef GAME_H
#define GAME_H

#include <pthread.h>
#include <sys/time.h>
#include "client.h"
extern unsigned int game_num;

typedef struct{
	int player_index[2];
	int game_index;
	int player_num;
	unsigned int state;
	pthread_mutex_t mtx_game;
	struct timeval timestamp;
	char *code;

		




}game_t;

void create_game(client_t *client);
void remove_game(game_t **game, client_t *skip);
void broadcast_game(game_t *game, char *msg, client_t *skip, int send_skip);
void clear_all_games();
game_t* get_game_by_index(unsigned int index);
void release_game(game_t *game);
void generate_game_code(char *code, unsigned int iteration);
game_t* get_game_by_code(char *code);
void remove_game(game_t **game, client_t *skip);
void leave_game(client_t *client);
int timeout_game(client_t *client);


#endif