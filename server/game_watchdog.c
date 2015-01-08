
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "game.h"
#include "global.h"
#include "logger.h"
/*LOG buffer*/
char log_buffer[LOG_BUFFER_SIZE];

/*
___________________________________________________________

    Game watchdog thread. Check every secont if some game 
    isn't timeouted or just one player is present in the
    game. Removes inactive games.
___________________________________________________________
*/
void *start_watchdog(void *arg) {
    pthread_mutex_t *mtx = (pthread_mutex_t *) arg;
    unsigned int got_games;
    int i;
    game_t *game;
    
    while(!stop_thread(mtx)) {
        got_games = 0;
        
        for(i = 0; i < MAX_CURRENT_CLIENTS && got_games <= game_num; i++) {
            game = get_game_by_index(i);
            
            if(game) {
                got_games++;
                

                if(game->state==1) {

                    if(game_time_before_timeout(game) < 0) {

                        if(game->player_num > 1) {

                            if(game_time_play_state_timeout(game)) {

                                sprintf(log_buffer,
                                        "Game with code %s and index %i TIMEOUT",
                                        game->code,
                                        game->game_index
                                        );
                                
                                log_line(log_buffer, LOG_DEBUG);
                                
                                broadcast_game(game, "GAME_LEFT;", NULL, 0);
                                
                                remove_game(&game, NULL);
                            }

                        }

                        else {

                            sprintf(log_buffer,
                                    "Game with code %s and index %i TIMEOUT",
                                    game->code,
                                    game->game_index
                                    );

                            log_line(log_buffer, LOG_DEBUG);
                            
                            broadcast_game(game, "GAME_LEFT;", NULL, 0);
                            
                            remove_game(&game, NULL);
                        }
                    }

                }

                else {
                    if(game_time_before_timeout(game) < 0) {

                        sprintf(log_buffer,
                                "Game with code %s and index %i TIMEOUT",
                                game->code,
                                game->game_index
                                );

                        log_line(log_buffer, LOG_DEBUG);
                        
                        broadcast_game(game, "GAME_LEFT;", NULL, 0);
                        
                        remove_game(&game, NULL);
                    }
                }
                
                if(game) {

                    release_game(game);
                }
            }
        }
        
        sleep(1);
    }
    
    log_line("SERV: Watchdog thread terminated.", LOG_ALWAYS);
    pthread_exit(NULL);
}
