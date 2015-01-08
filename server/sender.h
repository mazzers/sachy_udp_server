
#ifndef SENDER_H
#define	SENDER_H

#include <pthread.h>

extern pthread_cond_t cond_packet_change;

void *start_sending(void *arg);

#endif	/* SENDER_H */

