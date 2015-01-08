
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h> 
#include <sys/socket.h>
#include <errno.h> 
#include <netdb.h> 
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>

#include "logger.h"
#include "server.h"


char log_buffer[LOG_BUFFER_SIZE];

/*
___________________________________________________________

    Return random code with limited lenght
___________________________________________________________
*/
void gen_random(char *s, const int len) {
    int i;
    static const char alphanum[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    ;

    for (i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

/*
___________________________________________________________

    Stop thread for a while. 
___________________________________________________________
*/
int stop_thread(pthread_mutex_t *mtx) {

    switch(pthread_mutex_trylock(mtx)) {
        case 0:
            pthread_mutex_unlock(mtx);
            return 1;
        case EBUSY:
            return 0;
    }

    return 1;
}

/*
___________________________________________________________

    Conver hostname to ip. 
___________________________________________________________
*/
int hostname_to_ip(char *hostname, char *ip) {
    struct addrinfo hints, *res;
    struct sockaddr_in *res_addr;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;
    
    if( (getaddrinfo(hostname, NULL, &hints, &res) == 0) ) {
        if(res) {
            res_addr = (struct sockaddr_in *) res->ai_addr;
            strcpy(ip, inet_ntoa(res_addr->sin_addr));
            
            return 1;
        }
    }
    
    return 0;
}

/*
___________________________________________________________

    Display curent server uptime.
___________________________________________________________
*/
void display_uptime() {
    struct timeval temp_tv;
    
    gettimeofday(&temp_tv, NULL);


    sprintf(log_buffer,
            "Server uptime: %01.0fh:%02.0fm:%02.0fs",
            floor( (temp_tv.tv_sec - ts_start.tv_sec) / 3600.),
            floor(fmod((temp_tv.tv_sec - ts_start.tv_sec), 3600.0) / 60.0),
            fmod((temp_tv.tv_sec - ts_start.tv_sec), 60.0)
            );
    log_line(log_buffer, LOG_ALWAYS);
}
