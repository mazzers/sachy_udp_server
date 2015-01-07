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
	//printf("game.c: create_game\n");
	// char *message;
	char *buff;
	// unsigned int message_len;
	int i=0;

	if(client->game_index== -1){
		game_t *game = (game_t *) malloc(sizeof(game_t));
		gettimeofday(&game->timestamp,NULL);
		game->state=0;
		game->player_num=1;
		pthread_mutex_init(&game->mtx_game,NULL);

		memset(game->player_index, -1, sizeof(int)*2);
		game->player_index[0] = client->client_index;
		
		//printf("game.c mutex\n");
		
		//printf("here 0\n");
		game->code = (char *) malloc(GAME_CODE_LEN + 1);
		//game->host = (char *) malloc(NAME_LEN+1);
		//printf("Add game, host name:%s\n",client->name );
		generate_game_code(game->code, 0);
		//printf("here\n");

		if(game->code[0] != 0){
			for(i=0;i<MAX_CURRENT_CLIENTS;i++){
				if(games[i]==NULL){
					games[i]=game;
					game->game_index=i;
					//printf("game.c: add game with index %d\n",i );
					break;
				}


			}
			game_num++;
			//printf("game.c: games: %d\n", game_num);
			create_figures(game);
			//place_figures(game_t game);

			// sprintf(buff, "%d", ( GAME_MAX_LOBBY_TIME_SEC - 1));
			// message_len = strlen(buff) + GAME_CODE_LEN + 14 + 1;
			buff = (char *) malloc(14 + GAME_CODE_LEN);

			sprintf(buff, "GAME_CREATED;%s;",game->code);

			enqueue_dgram(client, buff, 1);

			//send_player_info(client,1);
			
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

game_t* get_game_by_code(char *code) {
	//printf("game.c: get_game_by_code\n");
	int i;

	for(i = 0; i < MAX_CURRENT_CLIENTS; i++) {
		//printf("%d\n",i );
		if(games[i]) {
			//printf("it's game with index: %d\n",i );
			pthread_mutex_lock(&games[i]->mtx_game);
			//printf("mutex lock get get_game_by_code\n");

			if(strncmp(code, games[i]->code, GAME_CODE_LEN) == 0) {
				return games[i];
			}

			pthread_mutex_unlock(&games[i]->mtx_game);
		}
	}

	return NULL;
}

void generate_game_code(char *code, unsigned int iteration) {
	//printf("game.c: generate_game_code\n");
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
	//printf("game.c: clear_all_games\n");
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
void remove_game(game_t **game, client_t *skip) {
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
void leave_game(client_t *client) {
	game_t *game;
	int i, n, k;
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

			//remove_game(&game);
			
			//client->game_index = -1;

			//}
			//else {

				// for(i = 0; i < 2; i++) {
			//enqueue_dgram(client, "GAME_LEFT", 1);

			if(game) {
                 // Release game 
				release_game(game);
			}
		}

	}
}


int timeout_game(client_t *client) {
	
	int i;
	char buff[40];
	game_t *game = get_game_by_index(client->game_index);

	if(game) {
		if(game->state) {
            /* Log */
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
            /* Release game */
			release_game(game);
		}
	}

	return 0;
}

int game_time_play_state_timeout(game_t *game) {
	struct timeval cur_tv;
	gettimeofday(&cur_tv, NULL);

	return ( (GAME_MAX_PLAY_STATE_TIME_SEC - (cur_tv.tv_sec - game->game_state.timestamp.tv_sec) ) <= 0);
}

int game_time_before_timeout(game_t *game) {
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

void join_game(client_t *client, char* game_code) {
	//printf("game.c: join_game\n");
	int i;
	game_t *game;
	char buff[20];

    /* Check if client is already in a game */
	if(client->game_index == -1) {
		game = get_game_by_code(game_code);
	}
	//printf("397\n");
	if(game) {
		//printf("399\n");
		if(!game->state) {
			//printf("400\n");
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
				          /* Set clients game index reference to this game */
				client->game_index = game->game_index;

                /* Send game state to joined client */
				//send_game_state(client, game);

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
				//send_player_info(client,2);

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

				enqueue_dgram(client, "GAME_FULL;", 1);
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

			enqueue_dgram(client, "GAME_RUNNING;", 1);
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

		enqueue_dgram(client, "GAME_NONEXISTENT;", 1);
	}
}

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

                /* Update game timestamp */
				gettimeofday(&game->timestamp, NULL);
                /* Update game state timestamp */
				gettimeofday(&game->game_state.timestamp, NULL);

			}
			release_game(game);
		}

	}

}

int can_player_move(client_t *client, game_t *game){
	if(client->color==game->game_state.playing){
		return 0;
	}else{
		return 1;
	}
}

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

// void broadcast_game_state(game_t *game){
// 	char buff[11+8];
// 	sprintf(buff,"STATE;%d;",game->game_state.playing);
// 	printf("after buffer\n");
// 	broadcast_game(game,buff,NULL,0);
// }

// void send_player_info(client_t *client, int color){
// 	//printf("send_player_info\n");
// 	char buff[21];
// 	if (client!=NULL)
// 	{
// 		/* code */

// 		if (color==1)
// 		{
			
// 			sprintf(buff,"YOU;WHITE;");
// 			enqueue_dgram(client,buff,1);

// 		}else{
// 			sprintf(buff,"YOU;BLACK;");
// 			enqueue_dgram(client,buff,1);

// 		}
// 	}
// }

void show_games(client_t *client){
	int i;
	char *buff;
	game_t *game;
	if (client!=NULL)
	{
		/* code */

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
						//printf("%s\n",games[i]->host);
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
			
			//printf("Game_num ==0\n");

		}else{
			game = get_game_by_index(client->game_index);
			buff=(char *) malloc(11+11+GAME_CODE_LEN);
			sprintf(buff,"HAS_GAME;%s;",game->code);
			enqueue_dgram(client,buff,1);
			free(buff);
			release_game(game);
		}
		//printf("Client has game already\n");

	}

}

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
		//printf("Start:%d/%d;Target:%d/%d\n",startRow,startCol,targetRow,targetCol );
		if (game)
		{	
			if(game->state){
				if(can_player_move(client,game)==0){


					i=validate_move(game,startRow,startCol,targetRow,targetCol );
					if (i==0)
					{
				//printf("Move is valid\n");

						figure_id=get_figure_by_row_col(game,startRow,startCol);
						target_figure_id=get_figure_by_row_col(game,targetRow,targetCol);
				//printf("Moving piece on server side\n");
				//if there is figure- set it as captured.
						if (game->game_state.fields[targetRow*8+targetCol]!=-1)
						{
							game->game_state.figures[target_figure_id]=1;
						}
						game->game_state.fields[startRow*8+startCol]=-1;
						game->game_state.fields[targetRow*8+targetCol]=figure_id;
						if (game->game_state.figures[4]==1)
						{
					//won black
							buff=(char *) malloc(11+10);
							sprintf(buff,"BLACK_WON;");
							//game->state=1;
							broadcast_game(game,buff,client,1);
							free(buff);

						}else if (game->game_state.figures[20]==1)
						{
							buff=(char *) malloc(11+10);
							sprintf(buff,"WHITE_WON;");
							//game->state=1;
							broadcast_game(game,buff,client,1);
							free(buff);
						}else{
							buff=(char *) malloc(11+25);
							sprintf(buff,"FIGURE_MOVED;%d;%d;%d;%d;",startRow,startCol,targetRow,targetCol);
							broadcast_game(game,buff,client,1);
							switch_state(game);
							sprintf(buff,"STATE;%d;",game->game_state.playing);
							broadcast_game(game,buff,client,1);
					//broadcast_game_state(game);
							free(buff);
						}

					}else{
				//printf("Move is invalid\n");
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

// void figure_moved(game_t *game, int startRow, int startCol){
// 	int i;
// 	char *buff;
// 	int figure;
// 	int figure_id;

// 	if (game)
// 	{
// 		figure_id=get_figure_by_row_col(game,startRow,startCol);
// 		if (figure_id>15)
// 		{
// 			printf("It's black figure\n");
// 			if (figure>=24)
// 			{
// 				printf("It's black pawn\n");
// 			}
// 		}else if(figure_id==-1){
// 			printf("No figure was found\n");
// 		}
// 		else{
// 			printf("It's white figure\n");
// 			if (figure>=8)
// 			{
// 				printf("It's white pawn\n");
// 			}
// 		}
// 		printf("Figure ID is:%d\n",figure_id);
// 			//release_game(game);
// 	}


// 		/* code */
// 	//}
// }

int validate_move(game_t *game,int startRow,int startCol,int targetRow,int targetCol){
	///add capture piece check, and won state
	
	
	int figure_id;
	figure_id = get_figure_by_row_col(game,startRow,startCol);
	if (figure_id==-1)
	{
		//figure don't exist
		
		return 1;
	}
	//moved piece match game state
	

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

int get_figure_by_row_col(game_t *game,int startRow, int startCol){
	//int* figure;
	int figure_id;
	figure_id=game->game_state.fields[startRow*8+startCol];
	//figure = game->game_state.figures[figure_id];
	return figure_id;

}

int is_rook_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol){
	int is_valid=1;
	//int side;
	if (target_is_free(game,targetRow,targetCol)==0 || target_is_capturable(game,startRow,startCol,targetRow,targetCol)==0)
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
			//white pawn
				

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
	}else if (target_is_capturable(game,startRow,startCol,targetRow,targetCol)==0)
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

int is_knight_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol){
	
	if (target_is_free(game,targetRow,targetCol)==0 || target_is_capturable(game,startRow,startCol,targetRow,targetCol)==0){

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
		/* code */
		return 0;
	}else if (startRow+2==targetRow && startCol-1==targetCol)
	{
		/* code */
		return 0;
	}else{
		return 1;
	}


}

int is_bishop_move_valid(game_t *game, int startRow,int startCol, int targetRow, int targetCol){
	int is_valid=1;
	if (target_is_free(game,targetRow,targetCol)==0 || target_is_capturable(game,startRow,startCol,targetRow,targetCol)==0)
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

int is_king_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol){
	int is_valid=1;
	if (target_is_free(game,targetRow,targetCol)==0 || target_is_capturable(game,startRow,startCol,targetRow,targetCol)==0)

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

int is_queen_move_valid(game_t *game,int startRow,int startCol,int targetRow,int targetCol){
	if(is_bishop_move_valid(game,startRow,startCol,targetRow,targetCol)==00 || is_rook_move_valid(game,startRow,startCol,targetRow,targetCol)==0){
		return 0;
	}else{
		return 1;
	}
	
	




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

int target_is_capturable(game_t *game, int startRow,int startCol,int targetRow,int targetCol){
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

int color_matched(int source_figure_id,int target_figure_id){
	if (source_figure_id<16 && target_figure_id <16 || source_figure_id>=16 && target_figure_id>=16)
	{
		//same color
		return 0;
	}else{
		//color differs
		return 1;
	}



}

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

int get_figure_field(game_t *game, int figure_id){
	int i;
	for(i=0;i<64;i++){
		if(game->game_state.fields[i]==figure_id){
			return i;
		}
	}
	return 100;
}

















// void send_game_state(client_t *client, game_t *game) {

// 	char *buff;
// 	unsigned short release = 0;
// 	unsigned short player[2] = {0};
// 	unsigned int i;
// 	int client_game_index;
// 	client_t *cur_client;

// 	if(client != NULL) {
// 		if(game == NULL) {
// 			game = get_game_by_index(client->game_index);

// 			release = 1;
// 		}

// 		if(game) {
//             /* If client is in that game */
// 			if(game->game_index == client->game_index) {
//                 /* Buffer is set to maximum possible size, but the actual message
//                  * is terminated by 0 so client can get the actual length
//                  */
//                  buff = (char *) malloc(105 + GAME_CODE_LEN + 11);

//                 /* Get players that are playing */
//                  for(i = 0; i < 2; i++) {
//                  	if(game->player_index[i] != -1) {

//                  		if(game->player_index[i] == client->client_index) {
//                  			client_game_index = i;

//                  			player[i] = 1;
//                  		}
//                  		else {
//                  			cur_client = get_client_by_index(game->player_index[i]);

//                  			if(cur_client) {
//                                 /* Client is active */
//                  				if(cur_client->state) {
//                  					player[i] = 1;
//                  				}
//                                 /* Client timeouted */
//                  				else {
//                  					player[i] = 2;
//                  				}

//                                 /* Release client */
//                  				release_client(cur_client);
//                  			}
//                  		}
//                  	}
//                  }

//                 /* game code, game state, 4x player connected, 16x figure position,
//                  * index of currently playing client, game index of connecting player
//                  * and timeout before next state change (lobby timeout, playing timeout)
//                  */
//                  sprintf(buff,
//                  	"GAME_STATE;%s;%u;%u;%u;%u;%d;%d;%d;%d:%d;%d;%d;%d:%d;%d;%d;%d:%d;%d;%d;%d:%d;%d;%d;%d:%d;%d;%d;%d:%d;%d;%d;%d:%d;%d;%d;%d:%d",
//                  	game->code,
//                  	game->state, 
//                  	player[0],
//                  	player[1],
//                  	client_game_index,
//                  	game->game_state.figures[0],
//                  	game->game_state.figures[1],
//                  	game->game_state.figures[2],
//                  	game->game_state.figures[3],
//                  	game->game_state.figures[4],
//                  	game->game_state.figures[5],
//                  	game->game_state.figures[6],
//                  	game->game_state.figures[7],
//                  	game->game_state.figures[8],
//                  	game->game_state.figures[9],
//                  	game->game_state.figures[10],
//                  	game->game_state.figures[11],
//                  	game->game_state.figures[12],
//                  	game->game_state.figures[13],
//                  	game->game_state.figures[14],
//                  	game->game_state.figures[15],
//                  	game->game_state.figures[16],
//                  	game->game_state.figures[17],
//                  	game->game_state.figures[18],
//                  	game->game_state.figures[19],
//                  	game->game_state.figures[20],
//                  	game->game_state.figures[21],
//                  	game->game_state.figures[22],
//                  	game->game_state.figures[23],
//                  	game->game_state.figures[24],
//                  	game->game_state.figures[25],
//                  	game->game_state.figures[26],
//                  	game->game_state.figures[27],
//                  	game->game_state.figures[28],
//                  	game->game_state.figures[29],
//                  	game->game_state.figures[30],
//                  	game->game_state.figures[31],
//                  	game_time_before_timeout(game)
//                  	);

// enqueue_dgram(client, buff, 1);

// free(buff);
// }

// if(release) {
//                 /* Release game */
// 	release_game(game);
// }
// }
// }
// }


void button_pushed(client_t *client, int button){
	//client_t *cur_client;
	game_t *game;
	char buff[13];
	if (button==1)
	{
		sprintf(buff, "BTN1_PUSHED;");
	}
	else{
		sprintf(buff, "BTN2_PUSHED;");
	}
 	//buff[11] = (char) (((int) ';'));
	buff[12] = 0;
	game = get_game_by_index(client->game_index);
 	//send_btn_pushed(client,game);
	broadcast_game(game, buff, client, 1);
	//free(buff);
	release_game(game);


}




void create_figures(game_t *game){
	int i;
	//empty field -1, if contain figure - its,index
	memset(game->game_state.fields,-1,(64 * sizeof(int)));
	//not captured 1, captured 0
	//memset(game->game_state.figures,1,(32 * sizeof(int)));
	

	for (i = 0; i < 8; ++i)
	{
		//white figures
		game->game_state.fields[i]=i;
		game->game_state.figures[i]=0;
		//white panws
		game->game_state.fields[8+i]=i+8;
		game->game_state.figures[8+i]=0;
		
		////black figures
		game->game_state.fields[56+i]=i+16;
		game->game_state.figures[16+i]=0;
		//black pawns
		game->game_state.fields[48+i]=i+24;
		game->game_state.figures[24+i]=0;
		
	}

	

}

int figure_by_type(int color,int type){
	if (color==0)
	{
		return type;
	}else{
		return type+10;
	}

}

 //void send_btn_pushed(client_t *client, game_t *game){
 	//char buff[13]


 //}
