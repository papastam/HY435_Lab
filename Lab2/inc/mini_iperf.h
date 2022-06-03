#ifndef MINI_IPERF_H
#define MINI_IPERF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <math.h>
#include "util_crc32.h"
#include "utils.h"
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <pthread.h>
#include <netdb.h>


#define DEFAULT_THREAD_PORT 53210
#define DEFAULT_UDP_PORT 55555
#define DEFAULT_TCP_PORT 55556
#define DEFAULT_ADDRESS "127.0.0.1"
#define TENTONINE 1000000000

#define TERMINATE_EXP 1

#define NTP_TIMESTAMP_DELTA 2208988800ull

// extern int udp_fd;
// extern int tcp_fd;

extern int *udp_fd_list;
extern int *tcp_fd_list;

/* Header for the tcp communication channel */
typedef struct tcp_header{
	uint16_t port; 					
	uint16_t experiment_duration; 	/* Experiment duration  seconds */
	uint16_t parallel_streams; 		/* number of parallel streams */
	uint16_t interval_duration; 		/* Interval to print experiment results */	
	uint32_t packet_length; 		/*  in Bytes  */
	uint32_t bandwidth; 			/* bits per seconds */
	uint16_t wait_duration; 		/* Wait before transmitting data */
	int 	 offset;
	uint16_t one_way_delay;
	uint16_t termination_signal;
}tcp_header_t;

typedef struct udp_header{ 
	uint32_t packet_id;   			/* to check outoforder and missing packets */
	uint32_t send_time; 				/* send time for one way delay */
	uint32_t checksum;
}udp_header_t; 

enum socket_type{UNDEFINED, SERVER, CLIENT};

typedef struct experiment_options{
	enum socket_type 	sock_type; 
	uint16_t 			interval_duration;
	int 				one_way_delay;
	char 				*address;
	uint16_t 			port;
	char 				*filename;
	uint32_t 			packet_length;
	uint32_t 			bandwidth;
	uint16_t 			experiment_duration;
	uint16_t 			wait_duration;
	uint16_t 			parallel_streams;
	uint16_t			offset;
	uint8_t 			isThread;
	int					NTPsocket;
}experiment_options_t;

typedef struct statistics{
	uint32_t 			socket_id;
	uint32_t 			throughput;
	uint32_t 			goodput;
	double 	 			pct_loss_percentage;
	uint64_t 			jitter;
	uint64_t			jitter_standard_deviation;
	double 				one_way_delay;
	struct timespec 	time;
	uint32_t 			sec_min;
	uint32_t 			sec_max;
}statistics_t;

typedef struct jitterDeviation{
	uint32_t sizeLimit;
	uint32_t fillSize;
	uint32_t *jitters_arr;
}jitter_deviation_t;


typedef struct bandwidthControl{
	uint32_t 			bandwidth;
	uint32_t 			sleeptime;
	struct timespec 	bandwidthEnd;
	uint32_t 			packet_id;
	uint16_t 			currsec;
	uint32_t 			payload_len;
	struct sockaddr_in 	sa;
}bandwidthControl_t;

typedef struct NTPmeasurement{
	time_t	arrival_time;
}NTPmeasurement_t;

typedef struct ntp_packet{
	uint8_t li_vn_mode;   	// Eight bits. li, vn, and mode.
                           // li.   Two bits.   Leap indicator.
                           // vn.   Three bits. Version number of the protocol.
                           // mode. Three bits. Client will pick mode 3 for client.
	 uint8_t stratum;         // Eight bits. Stratum level of the local clock.
    uint8_t poll;            // Eight bits. Maximum interval between successive messages.
    uint8_t precision;       // Eight bits. Precision of the local clock.

    uint32_t rootDelay;      // 32 bits. Total round trip delay time.
    uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
    uint32_t refId;          // 32 bits. Reference clock identifier.

    uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
    uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

    uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
    uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

    uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
    uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

    uint32_t txTm_s;         // 32 bits and the most important field the client cares about. 
												//Transmit time-stamp seconds.

    uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.
}ntp_packet_t;

int get_udp_socket(uint16_t offset);

int conduct_experiment_client_parallel(pthread_t *threads_arr, 
	const	experiment_options_t *inp_options);
int conduct_experiment_master_server(const experiment_options_t *inp_options);

int init_iperf_client(const experiment_options_t *measurement_options);

experiment_options_t* init_iperf_server(const experiment_options_t *measurement_options);

int terminate_iperf_server(const experiment_options_t *exp_options);
int terminate_iperf_client(int tcp_fd, int offset);

void die(char *err_msg, int offset);

void printTCPheader(tcp_header_t *header);
void printOptions(experiment_options_t *exp_options, char* who);

void printStatisticsPerSeconds(const statistics_t *statistics, uint16_t offset, int file_fd);

int start_experiment_client(int sock, const experiment_options_t *input_options);
int start_experiment_server(int sock, const experiment_options_t *input_options);

int conduct_experiment_client(const experiment_options_t* input_options);
int conduct_experiment_server(const experiment_options_t* input_options);

void clean_up(int tcp_fd, int udp_fd);

void setUp_required_options(experiment_options_t *input_options);

ssize_t send_stats(const statistics_t *stats, int offset);

uint64_t timespec_diff_ms(const struct timespec *timeA, const struct timespec *timeB);
uint64_t timespec_diff_lowres(const struct timespec *timeA, const struct timespec *timeB);
uint64_t timespec_diff_highres(const struct timespec *timeA, const struct timespec *timeB);


uint32_t calculateOneWayDelay(const experiment_options_t *exp_options);
void output_to_file(const char *filename, const statistics_t *stats);
uint64_t calculateJitterStandardDeviation(uint64_t *jitters);

ssize_t send_wbandwidth(bandwidthControl_t *controlstr, int offset);

void *start_thread_exp(void* exp_opt);

int initNTPconnection(int offset);
time_t getTimeFromNTP(int NTPsocket,int offset);


#endif 
