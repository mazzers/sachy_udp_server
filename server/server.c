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
#include "client.h"




int server_sockfd,client_sockfd;

void init_server(){
	int server_len, client_len;
	int n;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	char dgram[512];

	server_sockfd =socket(AF_INET,SOCK_DGRAM,0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(10000);

	server_len = sizeof(server_addr);
	client_len = sizeof(client_len);

	if (bind(server_sockfd,(struct sockaddr *) &server_addr, server_len)!=0){
		printf("error bind");

	}

	while(1){
			printf("Server ceka\n");
			n = recvfrom(server_sockfd,&dgram,sizeof(dgram),0,(struct sockaddr *) &client_addr, &client_len);
			printf("Got data\n%s\n",dgram );
			read_received_mgs(dgram, &client_addr);







	}




}

void read_received_mgs(char *dgram, struct sockaddr_in *addr){
	char *msg;
	msg = strtok(dgram,";");
	if (strncmp(msg,"ADD_CLIENT",10)==0)
	{
		printf("Call: Add client\n");
		add_client(addr);
	}else if (strncmp(msg,"CREATE_GAME",11)==0){
		printf("Create game\n");
	}else if (strncmp(msg,"JOIN_GAME",9)==0){
		printf("Join game\n");
	}


}
