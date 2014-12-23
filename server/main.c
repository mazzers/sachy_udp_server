#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "server.h"
#include "err.h"
#include "game.h"
#include "logger.h"
#include "global.h"
#include "receiver.h"

pthread_t thr_receiver; 
pthread_mutex_t mtx_thr_receiver; 
char log_buffer[LOG_BUFFER_SIZE];


int run(){
	char user_input_buffer[250];



	gettimeofday(&ts_start,NULL);
	init_logger("logfile.log");
	log_line("parapappaa", LOG_ALWAYS);



	init_server("127.0.0.1",10000);

	pthread_mutex_init(&mtx_thr_receiver, NULL);
	pthread_mutex_lock(&mtx_thr_receiver);
	
	if(pthread_create(&thr_receiver, NULL, start_receiving, (void *) &mtx_thr_receiver) != 0) {
		raise_error("Error starting receiving thread.");
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


int main(int argc, char const *argv[])
{
	run();

	return (EXIT_SUCCESS);
}

