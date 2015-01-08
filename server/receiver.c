

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>

#include "global.h"
#include "server.h"
#include "logger.h"
#include "err.h"
#include "com.h"

/*
___________________________________________________________

    Receiver thread. Receive new datagrams and 
    send it do server.
___________________________________________________________
*/
void *start_receiving(void *arg) {
    unsigned int client_len;
    int n;
    struct sockaddr_in client_addr;
    char dgram[MAX_DGRAM_SIZE];
    pthread_mutex_t *thr_mutex = (pthread_mutex_t *) arg;
    
    client_len = sizeof(client_addr);

    srand(time(NULL));

    while(!stop_thread(thr_mutex)) {
        n = recvfrom(server_sockfd, &dgram, sizeof(dgram), 0,
                (struct sockaddr *) &client_addr, &client_len);
        

        if(n > 0) {        
            process_dgram(dgram, &client_addr);
            

            recv_bytes += n;
            recv_dgrams++;
        }
    }
    
    log_line("SERV: Receiving thread terminated.", LOG_ALWAYS);
    
    pthread_exit(NULL);
}
