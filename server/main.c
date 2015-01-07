#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <arpa/inet.h>

#include "server.h"
#include "err.h"
#include "game.h"
#include "game_watchdog.h"
#include "logger.h"
#include "global.h"
#include "receiver.h"
#include "sender.h"
#include "com.h"

pthread_t thr_receiver; 
pthread_t thr_sender;
pthread_t thr_watchdog;

pthread_mutex_t mtx_thr_receiver; 
pthread_mutex_t mtx_thr_sender;
pthread_mutex_t mtx_thr_watchdog;
char log_buffer[LOG_BUFFER_SIZE];


int run(){
	char user_input_buffer[250];



	gettimeofday(&ts_start,NULL);
	init_logger("logfile.log");
	//log_line("parapappaa", LOG_ALWAYS);



	init_server("127.0.0.1",10000);

    pthread_mutex_init(&mtx_thr_watchdog, NULL);
    pthread_mutex_lock(&mtx_thr_watchdog);
    
    if(pthread_create(&thr_watchdog, NULL, start_watchdog, (void *) &mtx_thr_watchdog) != 0) {
        raise_error("Error starting watchdog thread.");
    }

	pthread_mutex_init(&mtx_thr_receiver, NULL);
	pthread_mutex_lock(&mtx_thr_receiver);

	if(pthread_create(&thr_receiver, NULL, start_receiving, (void *) &mtx_thr_receiver) != 0) {
		raise_error("Error starting receiving thread.");
	}

	   pthread_mutex_init(&mtx_thr_sender, NULL);
    pthread_mutex_lock(&mtx_thr_sender);
    
    if(pthread_create(&thr_sender, NULL, start_sending, (void *) &mtx_thr_sender) != 0) {
        raise_error("Error starting sender thread.");
    }
    

	while(1){
		if(fgets(user_input_buffer, 250, stdin) != NULL) {
            /* Exit server with exit, shutdown, halt or close commands */
			if( (strncmp(user_input_buffer, "exit", 4) == 0) ||
				(strncmp(user_input_buffer, "shutdown", 8) == 0) ||
				(strncmp(user_input_buffer, "halt", 4) == 0) ||
				(strncmp(user_input_buffer, "close", 5) == 0)) {

                //_shutdown();
				return(0);
			break;                
		}
		
	}
}


}


void _shutdown() {        
    char *msg = "CONN_CLOSE";    
    
    log_line("SERV: Caught shutdown command.", LOG_ALWAYS);
    log_line("SERV: Informing clients server is going down.", LOG_ALWAYS);
    
    /* Inform clients about shutdown */
    broadcast_clients(msg);
    
    log_line("#### START Stats ####", LOG_ALWAYS);
    
    /* Elapsed time */
    display_uptime();
    
    /* Sent bytes*/
    sprintf(log_buffer,
            "Sent bytes (raw): %u",
            sent_bytes
            );
    log_line(log_buffer, LOG_ALWAYS);
    
    /* Sent messages */
    sprintf(log_buffer,
            "Sent datagrams: %u",
            sent_dgrams
            );
    log_line(log_buffer, LOG_ALWAYS);
    
    /* Received bytes */
    sprintf(log_buffer,
            "Received bytes (raw): %u",
            recv_bytes
            );
    log_line(log_buffer, LOG_ALWAYS);
    
    /* Received messages */
    sprintf(log_buffer,
            "Received datagrams: %u",
            recv_dgrams
            );
    log_line(log_buffer, LOG_ALWAYS);
    
    /* Total number of connections */
    sprintf(log_buffer,
            "Total # of connections: %u",
            num_connections
            );
    log_line(log_buffer, LOG_ALWAYS);
    
    log_line("#### END Stats ####", LOG_ALWAYS);
    
    /* Clear clients */
    clear_all_clients();
    /* Clear games */
    clear_all_games();
    
    log_line("SERV: Asking threads to terminate.", LOG_ALWAYS);
    
    //pthread_mutex_unlock(&mtx_thr_watchdog);
    pthread_mutex_unlock(&mtx_thr_receiver);
    pthread_mutex_unlock(&mtx_thr_sender);
    
    /* Join threads */
    //pthread_join(thr_watchdog, NULL);
    pthread_join(thr_receiver, NULL);
    pthread_join(thr_sender, NULL);
    
    stop_logger();
}





int main(int argc, char const *argv[])
{
	run();

	return (EXIT_SUCCESS);
}

