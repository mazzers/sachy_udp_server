#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "client.h"
#include "global.h"
#include "logger.h"
#include "com.h"
/*Number of current games*/
unsigned int game_num=0;
/*Log buffer*/
char log_buffer[LOG_BUFFER_SIZE];
/*Array of games*/
game_t *games[MAX_CURRENT_CLIENTS];

/*
___________________________________________________________

    Create new game. Allocate memmory. Set client game_index
    to game index. 
___________________________________________________________
*/
void create_game(client_t *client){
	char *buff;
	int i=0;
	if(client->game_index== -1){
		game_t *game = (game_t *) malloc(sizeof(game_t));
		gettimeofday(&game->timestamp,NULL);
		game->state=0;
		game->player_num=1;
		pthread_mutex_init(&game->mtx_game,NULL);

		memset(game->player_index, -1, sizeof(int)*2);
		game->player_index[0] = client->client_index;
		
		game->code = (char *) malloc(GAME_CODE_LEN + 1);
		generate_game_code(game->code, 0);

		if(game->code[0] != 0){
			for(i=0;i<MAX_CURRENT_CLIENTS;i++){
				if(games[i]==NULL){
					games[i]=game;
					game->game_index=i;

					break;
				}
			}
			game_num++;
			create_figures(game);
			buff = (char *) malloc(14 + GAME_CODE_LEN);

			sprintf(buff, "GAME_CREATED;%s;",game->code);
			enqueue_dgram(client, buff, 1);
			
			sprintf(log_buffer,
				"Created new game with code %s and index %d",
				game->code,
				game->game_index
				);
			log_line(log_buffer, LOG_DEBUG);

			client->game_index = game->game_index;
			free(buff);
		}
	}
}

/*
___________________________________________________________

    Return game by game code. 
___________________________________________________________
*/
game_t* get_game_by_code(char *code) {
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

/*
___________________________________________________________

    Generate code for new game.
___________________________________________________________
*/
void generate_game_code(char *code, unsigned int iteration) {
	game_t *existing_game;
	/*prevent infinite loop*/
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

/*
___________________________________________________________

    Broadcast mesasge to all clients in game.
___________________________________________________________
*/
void broadcast_game(game_t *game, char *msg, client_t *skip, int send_skip) {
	int i;
	client_t *client;
	int release;

	if(game != NULL) {      
		for(i = 0; i < 2; i++) {
			client = NULL;
			release = 1;

			if(game->player_index[i] != -1) {                
				
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
						release_client(client);
					}
				}

			}
		}
	}
}

/*
___________________________________________________________

    Remove all games on server.
___________________________________________________________
*/
void clear_all_games() {
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

/*
___________________________________________________________

    Return game by index.
___________________________________________________________
*/
game_t* get_game_by_index(unsigned int index) {
	if(index>= 0 && MAX_CURRENT_CLIENTS > index) {
		if(games[index]) {
			pthread_mutex_lock(&games[index]->mtx_game);
			return games[index];
		}
	}
	return NULL;
}

/*
___________________________________________________________

    Release game mutex. 
___________________________________________________________
*/
void release_game(game_t *game) {
	if(game && pthread_mutex_trylock(&game->mtx_game) != 0) {
		pthread_mutex_unlock(&game->mtx_game);
	}
	else {
		log_line("Tried to release non-locked game", LOG_WARN);
	}
}

/*
___________________________________________________________

    Remove game.
___________________________________________________________
*/
void remove_game(game_t **game, client_t *skip) {
	int i;
	client_t *cur;
	if(game != NULL) {
		sprintf(log_buffer,
			"Removing game with code %s and index %d",
			(*game)->code,
			(*game)->game_index
			);
		log_line(log_buffer, LOG_DEBUG);

		for (i = 0; i < 2; i++)
		{
			if((*game)->player_index[i] != -1 && (!skip || (*game)->player_index[i] != skip->client_index)){
				cur = get_client_by_index((*game)->player_index[i]);
				if (cur)
				{
					cur->game_index=-1;
					cur->color=0;
					release_client(cur);
				}
			}
		}

		free((*game)->code);
		games[(*game)->game_index] = NULL;
		game_num--;

		release_game((*game));

		free((*game));
	}
	*game = NULL;
}

/*
___________________________________________________________

    Leave game. Set client game_index to -1. Removes game 
    if it's last player in the game. Send message about leaving.
___________________________________________________________
*/
void leave_game(client_t *client) {
	game_t *game;
	int i, n, k;
	int len;
	char *buff;

	if(client != NULL) {
		game = get_game_by_index(client->game_index);

		if(game) {
			sprintf(log_buffer,
				"Client with index %d is leaving game with code %s and index %d",
				client->client_index,
				game->code,
				game->game_index
				);

			log_line(log_buffer, LOG_DEBUG);

			if (game->player_num==1)
			{
				remove_game(&game,client);
			}else{
				for (i = 0; i < 2; i++)
				{
					if (game->player_index[i] ==client->client_index)
					{
						game->player_index[i]==-1;
						game->player_num--;

						break;

					}
				}
				game->state=0;
				buff = (char *) malloc(11+11);

				sprintf(buff, "CLIENT_LEFT;");

				broadcast_game(game, buff, client, 0);

				free(buff);
			}
			
			
			client->game_index=-1;
			client->color=0;

			enqueue_dgram(client,"GAME_LEFT;",1);
			if(game) {
				release_game(game);
			}
		}

	}
}

/*
___________________________________________________________

    Timeout game by last activity timestamp.
___________________________________________________________
*/
int timeout_game(client_t *client) {
	
	int i;
	char buff[40];
	game_t *game = get_game_by_index(client->game_index);

	if(game) {
		if(game->state) {
			sprintf(log_buffer,
				"Client with index %d timeouted from game with code %s and index %d",
				client->client_index,
				game->code,
				game->game_index
				);

			log_line(log_buffer, LOG_DEBUG);

			if(game->player_num>1){
				for(i=0; i<2;i++){
					if(game->player_index[i]==client->client_index){
						break;
					}
				}
				sprintf(buff, 
					"CLIENT_TIMEOUT;");

				broadcast_game(game, buff, client, 0);
				update_client_timestamp(client);
				release_game(game);

				return 1;

			}else{
				remove_game(&game,client);
			}
		}

		if(game) {
			release_game(game);
		}
	}

	return 0;
}

/*
___________________________________________________________

    Check if game is timeouted.
___________________________________________________________
*/
int game_time_play_state_timeout(game_t *game) {
	struct timeval cur_tv;
	gettimeofday(&cur_tv, NULL);

	return ( (GAME_MAX_PLAY_STATE_TIME_SEC - (cur_tv.tv_sec - game->game_state.timestamp.tv_sec) ) <= 0);
}

/*
___________________________________________________________

    Return time to timeout.
___________________________________________________________
*/
int game_time_before_timeout(game_t *game) {
	struct timeval cur_tv;    
	gettimeofday(&cur_tv, NULL);

	if(game->state) {
		return (GAME_MAX_PLAY_TIME_SEC - (cur_tv.tv_sec - game->timestamp.tv_sec));
	}
	else {
		return (GAME_MAX_LOBBY_TIME_SEC - (cur_tv.tv_sec - game->timestamp.tv_sec));
	}
}

/*
___________________________________________________________

    Join created game. Can't join running game. Notify 
    other player about connecting. Inform client about
    in case of error.
___________________________________________________________
*/
void join_game(client_t *client, char* game_code) {
	int i;
	game_t *game;
	char buff[20];

	if(client->game_index == -1) {
		game = get_game_by_code(game_code);
	}
	if(game) {
		if(!game->state) {
			if(game->player_num < 2) {

				
				for(i = 0; i < 2; i++) {
					if(game->player_index[i] == -1) {
						game->player_index[i] = client->client_index;

						break;
					}
				}


				strcpy(buff, "CLIENT_JOINED_GAME;");
				client->game_index = game->game_index;

				broadcast_game(game, buff, client, 1);

				game->player_num++;

				sprintf(log_buffer,
					"Player with index %d joined game with code %s and index %d",
					client->client_index,
					game->code,
					game->game_index
					);

				log_line(log_buffer, LOG_DEBUG);

			}
			else {
				sprintf(log_buffer,
					"Client with index %d tried to join game with code %s and index %d, but game was full",
					client->client_index,
					game->code,
					game->game_index
					);

				log_line(log_buffer, LOG_DEBUG);

				enqueue_dgram(client, "GAME_FULL;", 1);
			}
		}
		else {
			sprintf(log_buffer,
				"Client with index %d tried to join game with code %s and index %d, but game was already running",
				client->client_index,
				game->code,
				game->game_index
				);

			log_line(log_buffer, LOG_DEBUG);

			enqueue_dgram(client, "GAME_RUNNING;", 1);
		}

		release_game(game);
	}
	else {
		sprintf(log_buffer,
			"Client with index %d tried to join game with code %s, but game DOESNT EXIST",
			client->client_index,
			game_code
			);

		log_line(log_buffer, LOG_DEBUG);

		enqueue_dgram(client, "GAME_NONEXISTENT;", 1);
	}
}

/*
___________________________________________________________

    Start game. Set game state to running. Notify clients.
    Set playing player.
___________________________________________________________
*/
void start_game(client_t *client){
	game_t *game = NULL;
	char *buff;
	int i;
	if(client->game_index!=-1){
		game = get_game_by_index(client->game_index);
		if (game)
		{
			if(!game->state && game->player_num==2){
				sprintf(log_buffer,
					"Client with index %d started game with code %s and index %d",
					client->client_index,
					game->code,
					game->game_index
					);

				log_line(log_buffer, LOG_DEBUG);

				game->state=1;

				game->game_state.playing=STATE_WHITE;




				buff = (char *) malloc(16 + 11);

				enqueue_dgram(client,"YOU;WHITE;",1);
				broadcast_game(game,"YOU;BLACK;",client,0);

				sprintf(buff, 
					"GAME_STARTED;");

				broadcast_game(game, buff, client, 1);

				free(buff);

				gettimeofday(&game->timestamp, NULL);
				gettimeofday(&game->game_state.timestamp, NULL);

			}
			release_game(game);
		}

	}

}

/*
___________________________________________________________

    Check if it is players turn.
___________________________________________________________
*/
int can_player_move(client_t *client, game_t *game){
	if(client->color==game->game_state.playing){
		return 0;
	}else{
		return 1;
	}
}

/*
___________________________________________________________

   Switching between WHITE and BLACK states. Update game_state
   timestamp.
___________________________________________________________
*/
void switch_state(game_t *game){
	
	int curr = game->game_state.playing;
	if (curr==STATE_WHITE)
	{
		
		game->game_state.playing=STATE_BLACK;
	}else{
		
		game->game_state.playing=STATE_WHITE;
	}
	gettimeofday(&game->game_state.timestamp, NULL);

}

/*
___________________________________________________________

    Return current gamelist to client. NO_GAMES if there
    are no games on server at the moment. If player is in
    game already notify him about it.
___________________________________________________________
*/
void show_games(client_t *client){
	int i;
	char *buff;
	game_t *game;
	if (client!=NULL)
	{

		if (client->game_index==-1)
		{	
			if (game_num!=0)
			{
				buff=(char *) malloc(11+12+8*game_num);
				sprintf(buff,"GAME_LIST;%d;",game_num);
				for (i = 0; i < MAX_CURRENT_CLIENTS; i++)
				{

					if (games[i]!=NULL)
					{
						sprintf(buff+strlen(buff),"%s;%d;",games[i]->code,games[i]->state);
					}
				}
				enqueue_dgram(client,buff,1);
				free(buff);

			}else{
				buff=(char *) malloc(11+10);
				sprintf(buff,"NO_GAMES;");
				enqueue_dgram(client,buff,1);
				free(buff);
			}
			
		}else{
			game = get_game_by_index(client->game_index);
			buff=(char *) malloc(11+11+GAME_CODE_LEN);
			sprintf(buff,"HAS_GAME;%s;",game->code);
			enqueue_dgram(client,buff,1);
			free(buff);
			release_game(game);
		}

	}

}

/*
___________________________________________________________

   Check if move is valid and whether on not king figure is 
   captured. If so, notify players about that. Update last
   game activity time.
___________________________________________________________
*/
void send_figure_moved(client_t *client,int targetCol,int targetRow, int startCol, int startRow){
	int i;
	int figure_id;
	int figure;
	int target_figure;
	int target_figure_id;
	game_t *game;
	char *buff;
	if (client!=NULL){
		game = get_game_by_index(client->game_index);
		if (game)
		{	
			if(game->state){
				if(can_player_move(client,game)==0){


					i=validate_move(game,startRow,startCol,targetRow,targetCol );
					if (i==0)
					{

						figure_id=get_figure_by_row_col(game,startRow,startCol);
						target_figure_id=get_figure_by_row_col(game,targetRow,targetCol);
						if (game->game_state.fields[targetRow*8+targetCol]!=-1)
						{
							game->game_state.figures[target_figure_id]=1;
						}
						game->game_state.fields[startRow*8+startCol]=-1;
						game->game_state.fields[targetRow*8+targetCol]=figure_id;
						if (game->game_state.figures[4]==1)
						{
							buff=(char *) malloc(11+10);
							sprintf(buff,"BLACK_WON;");
							broadcast_game(game,buff,client,1);
							free(buff);

						}else if (game->game_state.figures[20]==1)
						{
							buff=(char *) malloc(11+10);
							sprintf(buff,"WHITE_WON;");
							broadcast_game(game,buff,client,1);
							free(buff);
						}else{
							buff=(char *) malloc(11+25);
							sprintf(buff,"FIGURE_MOVED;%d;%d;%d;%d;",startRow,startCol,targetRow,targetCol);
							broadcast_game(game,buff,client,1);
							switch_state(game);
							sprintf(buff,"STATE;%d;",game->game_state.playing);
							broadcast_game(game,buff,client,1);
							free(buff);
						}

					}else{
						buff=(char *) malloc(11+10);
						sprintf(buff,"BAD_MOVE;");
						enqueue_dgram(client,buff,1);
						free(buff);
					}
					gettimeofday(&game->timestamp, NULL);
					gettimeofday(&game->game_state.timestamp, NULL);
				}
			}

			release_game(game);	
		}

	}
}

/*
___________________________________________________________

    Return 0 if move is valid, otherwise return 1. 
___________________________________________________________
*/
int validate_move(game_t *game,int startRow,int startCol,int targetRow,int targetCol){
	
	
	int figure_id;
	figure_id = get_figure_by_row_col(game,startRow,startCol);
	if (figure_id==-1)
	{
		
		return 1;
	}
	

	if (targetRow < 0 || targetRow > 7 || targetCol<0 || targetCol>7)
	{
		
		return 1;
	}

	int move_is_valid=1;
	if (figure_id==0 || figure_id==7 || figure_id==16 || figure_id ==23)
	{	
		move_is_valid=is_rook_move_valid(game,startRow,startCol,targetRow,targetCol);
	}else if (figure_id>=8&&figure_id<16 || figure_id>=24 && figure_id<32)
	{	
		move_is_valid=is_pawn_move_valid(game,startRow,startCol,targetRow,targetCol);
	}else if (figure_id==2 || figure_id==5 || figure_id == 18 || figure_id==21)
	{

		move_is_valid=is_bishop_move_valid(game,startRow,startCol,targetRow,targetCol);
	}else if (figure_id==4 || figure_id==20)
	{

		move_is_valid=is_king_move_valid(game,startRow,startCol,targetRow,targetCol);
	}else if (figure_id==3 || figure_id==19)
	{

		move_is_valid=is_queen_move_valid(game,startRow,startCol,targetRow,targetCol);
	}else if (figure_id==1 || figure_id==6 || figure_id==17 || figure_id==22)
	{	
		move_is_valid=is_knight_move_valid(game,startRow,startCol,targetRow,targetCol);
	}

	return move_is_valid;

}

/*
___________________________________________________________

    Return figure_id by it's position. 
___________________________________________________________
*/
int get_figure_by_row_col(game_t *game,int startRow, int startCol){
	int figure_id;
	figure_id=game->game_state.fields[startRow*8+startCol];

}

/*
___________________________________________________________

    Check if rook move is valid. 0-valid. 1-invalid.
___________________________________________________________
*/
int is_rook_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol){
	int is_valid=1;
	if (target_is_free(game,targetRow,targetCol)==0 || target_is_captureable(game,startRow,startCol,targetRow,targetCol)==0)
	{


	}else{
		return 1;
	}
	int diffRow = targetRow - startRow;
	int diffCol = targetCol - startCol;
	if (diffRow==0 && diffCol>0)
	{
		if (are_pieces_between_source_and_target(game,startRow,startCol,targetRow,targetCol,0,+1)!=0)
		{
			is_valid=0;
		}
		
	}else if (diffRow ==0 && diffCol <0){
		if (are_pieces_between_source_and_target(game,startRow,startCol,targetRow,targetCol,0,-1)!=0)
		{
			is_valid=0;
		}
		
	}else if (diffRow>0 && diffCol==0)
	{	if(are_pieces_between_source_and_target(game,startRow,startCol,targetRow,targetCol,+1,0)!=0){
		is_valid=0;
	}

}else if (diffRow<0 && diffCol ==0)
{	
	if (are_pieces_between_source_and_target(game,startRow,startCol,targetRow,targetCol,-1,0)!=0)
	{
		is_valid=0;
	}

}else{
	is_valid = 1;
}
return is_valid;
}

/*
___________________________________________________________

	Check if pawn move is valid. 0-valid. 1-invalid.
___________________________________________________________
*/
int is_pawn_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol){
	
	int is_valid=1;
	int figure_id;
	figure_id = get_figure_by_row_col(game,startRow,startCol);
	if (target_is_free(game,targetRow,targetCol)==0)
	{
		
		if (startCol==targetCol)
		{
			
			if (figure_id<16)
			{
				

				if (startRow+1 ==targetRow)
				{
					
					is_valid=0;
				}else{
					
					is_valid=1;
				}
			}else{
				
				if (startRow-1==targetRow)
				{
					
					is_valid=0;
				}else{
					
					is_valid=1;
				}
			}

		}else{
			
			is_valid=1;
		}
	}else if (target_is_captureable(game,startRow,startCol,targetRow,targetCol)==0)
	{	
		
		if (startCol+1==targetCol || startCol-1==targetCol)
		{
			if (figure_id<16)
			{
				if (startRow+1==targetRow)
				{
					is_valid=0;
				}else
				{
					is_valid=1;
				}
			}else{
				if (startRow-1==targetRow)
				{
					is_valid=0;
				}else{
					is_valid=1;
				}
			}
		}else{
			is_valid=1;
		}
	}
	
	return is_valid;



}

/*
___________________________________________________________

    Check if knight move is valid.
___________________________________________________________
*/
int is_knight_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol){
	
	if (target_is_free(game,targetRow,targetCol)==0 || target_is_captureable(game,startRow,startCol,targetRow,targetCol)==0){

	}else
	{
		return 1;
	}
	if (startRow+2 ==targetRow && startCol+1==targetCol)
	{
		return 0;
	}else if (startRow+1==targetRow && startCol+2==targetCol)
	{
		return 0;
	}else if (startRow-1==targetRow && startCol+2 ==targetCol)
	{
		return 0;
	}else if (startRow-2==targetRow && startCol+1==targetCol)
	{
		return 0;
	}else if (startRow-2==targetRow && startCol-1==targetCol)
	{
		return 0;
	}else if (startRow-1==targetRow && startCol-2==targetCol)
	{
		return 0;
	}else if (startRow+1==targetRow && startCol-2==targetCol)
	{
		return 0;
	}else if (startRow+2==targetRow && startCol-1==targetCol)
	{
		return 0;
	}else{
		return 1;
	}


}

/*
___________________________________________________________

    Check if bishop move is valid.
___________________________________________________________
*/
int is_bishop_move_valid(game_t *game, int startRow,int startCol, int targetRow, int targetCol){
	int is_valid=1;
	if (target_is_free(game,targetRow,targetCol)==0 || target_is_captureable(game,startRow,startCol,targetRow,targetCol)==0)
	{
		
	}else{
		return 1;
	}
	int diffRow = targetRow-startRow;
	int diffCol = targetCol-startCol;
	if (diffRow ==diffCol && diffCol>0)
	{
		if (are_pieces_between_source_and_target(game,startRow,startCol,targetRow,targetCol,+1,+1)==0)
		{
			is_valid=1;
		}else{
			is_valid=0;
		}
		
	}else if (diffRow==-diffCol && diffCol>0)
	{
		if (are_pieces_between_source_and_target(game,startRow,startCol,targetRow,targetCol,-1,+1)==0)
		{
			is_valid=1;
		}else{
			is_valid=0;
		}
	}else if(diffRow==diffCol && diffCol < 0){
		if (are_pieces_between_source_and_target(game,startRow,startCol,targetRow,targetCol,-1,-1)==0)
		{
			is_valid=1;
		}else{
			is_valid=0;
		}
	}else if (diffRow==-diffCol && diffCol < 0)
	{
		if (are_pieces_between_source_and_target(game,startRow,startCol,targetRow,targetCol,+1,-1)==0)
		{
			is_valid=1;
		}else{
			is_valid=0;
		}
	}else{
		is_valid=1;
	}
	return is_valid;

}

/*
___________________________________________________________

    check if king move is valid.
___________________________________________________________
*/
int is_king_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol){
	int is_valid=1;
	if (target_is_free(game,targetRow,targetCol)==0 || target_is_captureable(game,startRow,startCol,targetRow,targetCol)==0)

	{
		
	}else{
		return 1;
	}
	if (startRow+1 ==targetRow && startCol==targetCol)
	{
		is_valid=0;
	}else if (startRow+1==targetRow&&startCol+1==targetCol)
	{
		is_valid=0;
	}else if(startRow==targetRow && startCol+1==targetCol){
		is_valid=0;
	}else if (startRow-1==targetRow && startCol+1==targetCol)
	{
		is_valid=0;
	}else if(startRow-1==targetRow && startCol==targetCol){
		is_valid=0;
	}else if (startRow-1==targetRow && startCol-1==targetCol)
	{
		is_valid=0;
	}else if (startRow==targetRow && startCol-1==targetCol)
	{
		is_valid=0;
	}else if (startRow+1==targetRow && startCol-1==targetCol)
	{
		is_valid=0;
	}else{
		is_valid=1;
	}

	return is_valid;
}

/*
___________________________________________________________

    Check if queen move is valid. 
___________________________________________________________
*/
int is_queen_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol){
	if(is_bishop_move_valid(game,startRow,startCol,targetRow,targetCol)==00 || is_rook_move_valid(game,startRow,startCol,targetRow,targetCol)==0){
		return 0;
	}else{
		return 1;
	}
	
/*
___________________________________________________________

    Check if target field is free.
___________________________________________________________
*/
}

int target_is_free(game_t *game,int targetRow,int targetCol){
	int figure_id;
	figure_id = get_figure_by_row_col(game,targetRow,targetCol);
	if (figure_id==-1)
	{
		return 0;
	}else{
		return 1;
	}


}

/*
___________________________________________________________

    Check if targer figure is captureable.
___________________________________________________________
*/
int target_is_captureable(game_t *game, int startRow,int startCol,int targetRow,int targetCol){
	int source_figure_id;
	int target_figure_id;
	source_figure_id = get_figure_by_row_col(game,startRow,startCol);
	target_figure_id = get_figure_by_row_col(game,targetRow,targetCol);
	if (target_figure_id==-1)
	{
		return 1;
	}else if (color_matched(source_figure_id,target_figure_id)==1){
		return 0;
	}else{
		return 1;
	}
}

/*
___________________________________________________________

    Check if target figure is same as moving one. 
___________________________________________________________
*/
int color_matched(int source_figure_id,int target_figure_id){
	if (source_figure_id<16 && target_figure_id <16 || source_figure_id>=16 && target_figure_id>=16)
	{
		/*same color*/
		return 0;
	}else{
		/*color differs*/
		return 1;
	}
}

/*
___________________________________________________________

    Check if there are other figures between start and target.
___________________________________________________________
*/
int are_pieces_between_source_and_target(game_t *game, int startRow,int startCol,int targetRow,int targetCol,int rowInc,int colInc){
	int curr_row = startRow + rowInc;
	int curr_col = startCol + colInc;
	while(1){
		if (curr_row==targetRow && curr_col==targetCol)
		{
			break;
		}
		if (curr_row < 0 || curr_row > 7 || curr_col<0 || curr_col>7)
		{
			break;
		}
		if (non_captured_figure_at_location(game,curr_row,curr_col)==0)
		{
			return 0;
		}
		curr_row +=rowInc;
		curr_col +=colInc;

	}
	return 1;
}

/*
___________________________________________________________

    Check if non captured figure is on field. 
___________________________________________________________
*/
int non_captured_figure_at_location(game_t *game,int targetRow,int targetCol){
	int figure_id;
	figure_id = get_figure_by_row_col(game,targetRow,targetCol);
	if (figure_id==-1)
	{
		return 1;
	}else{
		return 0;
	}


}

/*
___________________________________________________________

    Place figures on their start fields for new game. 
___________________________________________________________
*/
void create_figures(game_t *game){
	int i;
	memset(game->game_state.fields,-1,(64 * sizeof(int)));
	
	for (i = 0; i < 8; ++i)
	{
		//white figures
		game->game_state.fields[i]=i;
		game->game_state.figures[i]=0;
		//white panws
		game->game_state.fields[8+i]=i+8;
		game->game_state.figures[8+i]=0;	
		//black figures
		game->game_state.fields[56+i]=i+16;
		game->game_state.figures[16+i]=0;
		//black pawns
		game->game_state.fields[48+i]=i+24;
		game->game_state.figures[24+i]=0;
		
	}
}




