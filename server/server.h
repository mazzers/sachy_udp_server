#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

extern int server_sockfd;

void init_server();
void read_received_mgs(char *dgram, struct sockaddr_in *addr);

#endif