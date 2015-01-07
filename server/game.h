#ifndef GAME_H
#define GAME_H

#include <pthread.h>
#include <sys/time.h>
#include "client.h"

#define STATE_WHITE 1
#define STATE_BLACK 2

#define STATE_WHITE_WIN 3
#define STATE_BLACK_WIN 4

#define COLOR_WHITE 1
#define COLOR_BLACK 2

#define TYPE_ROOK 1
#define TYPE_KNIGHT 2
#define TYPE_BISHOP 3
#define TYPE_QUEEN 4
#define TYPE_KING 5
#define TYPE_PAWN 6

extern unsigned int game_num;



typedef struct 
{
	unsigned int fields[64];
	int figures[32];
	//char *last_move;
	struct timeval timestamp;
	unsigned int state;
	unsigned short playing;




}game_state_t;




typedef struct{
	int player_index[2];
	unsigned int game_index;
	unsigned short player_num;
	unsigned short state;
	pthread_mutex_t mtx_game;
	struct timeval timestamp;
	char *code;
	game_state_t game_state;

		




}game_t;



void create_game(client_t *client);
void remove_game(game_t **game, client_t *skip);
void broadcast_game(game_t *game, char *msg, client_t *skip, int send_skip);
void clear_all_games();
game_t* get_game_by_index(unsigned int index);
void release_game(game_t *game);
void generate_game_code(char *code, unsigned int iteration);
game_t* get_game_by_code(char *code);
void leave_game(client_t *client);
int timeout_game(client_t *client);
void send_game_state(client_t *client, game_t *game);
int game_time_before_timeout(game_t *game);
void button_pushed(client_t *client, int button);
int figure_by_type(int color,int type);
void create_figures(game_t *game);
void place_figures(game_t *game);
void start_game(client_t *client);
void show_games(client_t *client);
void ask_other_to_move(client_t *client);
void send_figure_moved(client_t *client,int targetCol,int targetRow, int startCol, int startRow);
void figure_moved(game_t *game, int startCol, int startRow);
int validate_move(game_t *game,int startRow,int startCol,int targetRow,int targetCol);
int get_figure_by_row_col(game_t *game,int startRow, int startCol);
int is_rook_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol);
int target_is_free(game_t *game,int targetRow,int targetCol);
int target_is_capturable(game_t *game, int startRow,int startCol,int targetRow,int targetCol);
int color_matched(int source_figure_id,int target_figure_id);
int are_pieces_between_source_and_target(game_t *game, int startRow,int startCol,int targetRow,int targetCol,int rowInc,int colInc);
int non_captured_figure_at_location(game_t *game,int targetRow,int targetCol);
int is_pawn_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol);
int is_bishop_move_valid(game_t *game, int startRow,int startCol, int targetRow, int targetCol);
int is_king_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol);
int is_queen_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol);
int is_knight_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol);
void set_next_payer(game_t *game);

void send_player_info(client_t *client, int color);
void broadcast_game_state(game_t *game);
void switch_state(game_t *game);
int game_time_play_state_timeout(game_t *game);
int game_time_before_timeout(game_t *game) ;
int get_figure_field(game_t *game, int figure_id);

#endif