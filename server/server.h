#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

extern int server_sockfd;

extern struct timeval ts_start;


void init_server();
void read_received_mgs(char *dgram, struct sockaddr_in *addr);

void process_dgram(char *dgram, struct sockaddr_in *addr);
void set_socket_timeout_linux();
#endif