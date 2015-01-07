
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "game.h"
#include "global.h"
#include "logger.h"

/* Logger buffer */
char log_buffer[LOG_BUFFER_SIZE];

/**
 * void *start_watchdog(void *arg)
 * 
 * Entry point for watchdog thread. Watchdog keeps looping and checking all
 * created games if any of them timeouted untill he is signalled by main thread
 * that he should finish.
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
                
                /* Game is running */
                if(game->state==1) {
                    /* If player pick timeouted */
                    if(game_time_before_timeout(game) < 0) {
                        /* Check if there is another player that can play */
                        if(game->player_num > 1) {
                            /* If game stayed in active state without anyone
                             *  playing for way too long 
                             */
                            if(game_time_play_state_timeout(game)) {
                                /* Log */
                                sprintf(log_buffer,
                                        "Game with code %s and index %i TIMEOUT",
                                        game->code,
                                        game->game_index
                                        );
                                
                                log_line(log_buffer, LOG_DEBUG);
                                
                                broadcast_game(game, "GAME_LEFT;", NULL, 0);
                                
                                remove_game(&game, NULL);
                            }
                            // else {
                            //     set_game_playing(game);
                                
                            //     broadcast_game_playing_index(game, NULL);
                            // }
                        }
                        /* Only one player, remove game */
                        else {
                            /* Log */
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
                    //game has ended
                }
                /* Game is in lobby */
                else {
                    if(game_time_before_timeout(game) < 0) {
                        /* Log */
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
                    /* Release game */
                    release_game(game);
                }
            }
        }
        
        /* Not much precision needed here, just sleep for a second */
        sleep(1);
    }
    
    log_line("SERV: Watchdog thread terminated.", LOG_ALWAYS);
    pthread_exit(NULL);
}
