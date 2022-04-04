#ifndef MINI_IPER_H_
#define MINI_IPERF_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>

extern enum socket_type{server, client}

extern int interval_seconds;
extern int one_way_delay;
extern char *address;
extern char *port;
extern char *filename;
extern enum socket_type;
extern enum sock_type;
extern uint32_t payload;
extern uint32_t bandwidth;
extern uint32_t duration;
extern uint32_t wait_seconds;
extern uint16_t parallel_streams;



#endif 
