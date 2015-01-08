#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "logger.h"

#include "server.h"
#include "game_watchdog.h"
#include "global.h"
#include "receiver.h"
#include "game.h"
#include "sender.h"
#include "com.h"

pthread_t thr_receiver; 
pthread_t thr_sender;
pthread_t thr_watchdog;

pthread_mutex_t mtx_thr_receiver; 
pthread_mutex_t mtx_thr_sender;
pthread_mutex_t mtx_thr_watchdog;
char log_buffer[LOG_BUFFER_SIZE];

/*
___________________________________________________________

    Display server help.
___________________________________________________________
*/
void info(){
    printf("================= Chess server =========================\n");
    printf("========================================================\n");
    printf("================= Help =================================\n");
    printf("\t server <ip> <port> [logfile] [log level] [verbose level] \n");
    printf("========================================================\n");
    printf("================= Examples =============================\n");
    printf("\t server 0.0.0.0 10000\n");
    printf("\t server 0.0.0.0 10000 logfile.log\n");
    printf("\t server 0.0.0.0 10000 logfile.log 4\n");
    printf("\t server 0.0.0.0 10000 logfile.log 4 3\n");
    printf("========================================================\n");
    printf("================= Arguments ============================\n");
    printf("\t <ip> - Bind IP or hostname.\n");
    printf("\t <port> - Bind port.\n");
    printf("\t [logfile] - Log output file.\n");
    printf("\t [log level] - Loging level(Default level: 2 - Warnings).\n");
    printf("\t [verbose level] - Verbose level(Default level: 3 - Info).\n");
    printf("========================================================\n");
    printf("================= Log/verbose levels ===================\n");
    printf("\t 0 - Important messages.\n");
    printf("\t 1 - Error messages.\n");
    printf("\t 2 - Warning messages.\n");
    printf("\t 3 - Information messages.\n");
    printf("\t 4 - Debugging messages.\n");
    printf("\t 5 - Everything.\n");
    printf("========================================================\n");
    printf("================= Commands =============================\n");
    printf("\t info/help - Display help.\n");
    printf("\t close/exit - Call server shutdown.\n");
    printf("\t set_log [n] - Set log level to level n.\n");
    printf("\t set_verbose [n] - Set verbose level to level n.\n");
    printf("\t uptime - Display server's uptime.\n");
    printf("\t players - Display connectios count.\n");

    printf("\n\n");
    

}


/*
___________________________________________________________

    Close all connection and shutdown server.
___________________________________________________________
*/
void _shutdown() {        
    char *msg = "CONN_CLOSE";    
    
    log_line("SERV: Shutdown server.", LOG_ALWAYS);
    log_line("SERV: Notify clients.", LOG_ALWAYS);
    
    /* Inform clients about shutdown */
    broadcast_clients(msg);
    
    log_line("================= Server stats ===================", LOG_ALWAYS);
    
    /* Elapsed time */
    display_uptime();
    
    /* Sent bytes*/
    sprintf(log_buffer,
        "Sent bytes (raw): %u.",
        sent_bytes
        );
    log_line(log_buffer, LOG_ALWAYS);
    
    /* Sent messages */
    sprintf(log_buffer,
        "Sent datagrams: %u.",
        sent_dgrams
        );
    log_line(log_buffer, LOG_ALWAYS);
    
    /* Received bytes */
    sprintf(log_buffer,
        "Received bytes (raw): %u.",
        recv_bytes
        );
    log_line(log_buffer, LOG_ALWAYS);
    
    /* Received messages */
    sprintf(log_buffer,
        "Received datagrams: %u.",
        recv_dgrams
        );
    log_line(log_buffer, LOG_ALWAYS);
    
    /* Total number of connections */
    sprintf(log_buffer,
        "Total # of connections: %u.",
        num_connections
        );
    log_line(log_buffer, LOG_ALWAYS);
    
    log_line("========================================================", LOG_ALWAYS);
    
    /* Clear clients */
    clear_all_clients();
    /* Clear games */
    clear_all_games();
    
    log_line("SERV: Notify threads to shutdown.", LOG_ALWAYS);
    
    pthread_mutex_unlock(&mtx_thr_watchdog);
    pthread_mutex_unlock(&mtx_thr_receiver);
    pthread_mutex_unlock(&mtx_thr_sender);

    pthread_join(thr_watchdog, NULL);
    pthread_join(thr_receiver, NULL);
    pthread_join(thr_sender, NULL);
    
    stop_logger();
}

/*
___________________________________________________________

    Starts server from input arguments. Create threads. 
    Works with further user commands.
___________________________________________________________
*/

int run(int argc, char **argv){
	char user_input_buff[250];
    char addr_buffer[INET_ADDRSTRLEN] = {0};
    char *buff;
    struct in_addr tmp_addr;
    int port;
    int tmp_num;

    gettimeofday(&ts_start,NULL);

    if(argc >= 4) {
        init_logger(argv[3]);
    }else {
        init_logger("logfile.log");
    }

    /* Validate input */
    if(argc >= 3) {

        /* Validate ip address */
        if(inet_pton(AF_INET, argv[1], (void *) &tmp_addr) <= 0 &&
            !hostname_to_ip(argv[1], addr_buffer)) {

            info();
        raise_error("Wrong adress.");
        

    }

    if(!addr_buffer[0]) {
        strcpy(addr_buffer, argv[1]);
    }

        /* Validate port */
    port = (int) strtoul(argv[2], NULL, 10);

    if(port <= 0 || port >= 65536) {
        info();
        raise_error("Port number is out of range.");
        

    }

    if(port <= 1024 && port != 0) {
        log_line("Binding port below 1024. May require administrator privileges.", LOG_ALWAYS);
    }

        /* Got log severity */
    if(argc >= 5) {
        log_level = (int) strtol(argv[4], NULL, 10);
    }

    sprintf(log_buffer,
        "Log level is %d",
        log_level
        );

    log_line(log_buffer, LOG_ALWAYS);

        /* Got verbose */
    if(argc == 6) {
        verbose_level = (int) strtol(argv[5], NULL, 10);
    }

    sprintf(log_buffer,
        "Verbose level is %d",
        verbose_level
        );

    log_line(log_buffer, LOG_ALWAYS);
}
else {
    info();
    raise_error("Wrong agruments.");

}

    /* Initiate server */
init_server(addr_buffer, port);


pthread_mutex_init(&mtx_thr_watchdog, NULL);
pthread_mutex_lock(&mtx_thr_watchdog);

if(pthread_create(&thr_watchdog, NULL, start_watchdog, (void *) &mtx_thr_watchdog) != 0) {
    raise_error("Error. Can't create watchdog thread.");
}

pthread_mutex_init(&mtx_thr_receiver, NULL);
pthread_mutex_lock(&mtx_thr_receiver);

if(pthread_create(&thr_receiver, NULL, start_receiving, (void *) &mtx_thr_receiver) != 0) {
  raise_error("Error. Can't create receiver thread.");
}

pthread_mutex_init(&mtx_thr_sender, NULL);
pthread_mutex_lock(&mtx_thr_sender);

if(pthread_create(&thr_sender, NULL, start_sending, (void *) &mtx_thr_sender) != 0) {
    raise_error("Error. Can't create sender thread.");
}


while(1) {
    printf("CMD: ");

    if(fgets(user_input_buff, 250, stdin) != NULL) {
            /* Exit server with exit, shutdown, halt or close commands */
        if( (strncmp(user_input_buff, "exit", 4) == 0) ||
            (strncmp(user_input_buff, "close", 5) == 0))  {

            _shutdown();

        break;                
    }            
            /* Change log level */
    else if(strncmp(user_input_buff, "set_log", 7) == 0) {
        if(strtok(user_input_buff, " ") != NULL) {
            buff = strtok(NULL, " ");

            if(buff) {
                tmp_num = (int) strtoul(buff, NULL, 10);

                if(tmp_num >= LOG_NONE || tmp_num <= LOG_ALWAYS) {
                    log_level = tmp_num;

                    sprintf(log_buffer,
                        "CMD: Changing log level to %d",
                        log_level
                        );

                    log_line(log_buffer, LOG_ALWAYS);
                }
            }
        }
    }else if ((strncmp(user_input_buff, "info", 4) == 0)||
    (strncmp(user_input_buff, "help", 4) == 0))
    {
        info();
    }

            /* Change log level */
    else if(strncmp(user_input_buff, "set_verbose", 11) == 0) {
        if(strtok(user_input_buff, " ") != NULL) {
            buff = strtok(NULL, " ");

            if(buff) {
                tmp_num = (int) strtoul(buff, NULL, 10);

                if(tmp_num >= LOG_NONE || tmp_num <= LOG_ALWAYS) {
                    verbose_level = tmp_num;

                    sprintf(log_buffer,
                        "CMD: Changing verbose level to %d",
                        verbose_level
                        );

                    log_line(log_buffer, LOG_ALWAYS);
                }
            }
        }
    }

            /* Display server uptime */
    else if(strncmp(user_input_buff, "uptime", 6) == 0) {
        display_uptime();
    }

            /* Display current number of clients (including timeouted) */
    else if(strncmp(user_input_buff, "players", 7) == 0) {
        sprintf(log_buffer,
            "Current number of clients (including timeouted) is %d",
            client_num
            );

        log_line(log_buffer, LOG_ALWAYS);
    }
}
}
}












int main(int argc, char **argv)
{
	run(argc, argv);

	return (EXIT_SUCCESS);
}

