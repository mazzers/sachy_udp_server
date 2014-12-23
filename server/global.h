#ifndef GLOBAL_H
#define GLOBAL_H

#include <pthread.h>
#include <arpa/inet.h>

#define MAX_CURRENT_CLIENTS 100
#define MAX_CURRENT_GAMES 5



/* Application token identifying packets */
#define APP_TOKEN A11B0529P
/* Number of maximum concurrent clients (should be divisible by 4) */
//#define MAX_CONCURRENT_CLIENTS 100
/* Largest possible received dgram, indicates buffer size */
#define MAX_DGRAM_SIZE 512
/* Maximum number of microseconds tolerable before resending packet */
#define MAX_PACKET_AGE_USEC 500000
/* Maximum number of seconds with no response from client before changing his state */
#define MAX_CLIENT_NORESPONSE_SEC 30
/* Maximum number of seconds client can be marked as inactive before removing */
#define MAX_CLIENT_TIMEOUT_SEC 120
/* Game code length */
#define GAME_CODE_LEN 5
/* Maximum time game is in state 0 */
#define GAME_MAX_LOBBY_TIME_SEC 3600
/* Maximum time player can take to play */
#define GAME_MAX_PLAY_TIME_SEC 45
/* Maximum time game can be in active state without anyone playing */
#define GAME_MAX_PLAY_STATE_TIME_SEC 180

#define NANOSECONDS_IN_SECOND 1000000000

#define _STRINGIFY(s) #s
#define STRINGIFY(s) _STRINGIFY(s)


/* Function prototypes */
void gen_random(char *s, const int len);
int stop_thread(pthread_mutex_t *mtx);
int rand_lim(int limit);
int hostname_to_ip(char *hostname, char *ip);
void display_uptime();



#endif