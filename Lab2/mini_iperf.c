#include "mini_iperf.h"

enum socket_type{server, client}

int interval_seconds, opt, one_way_delay;
char *address, *port, *filename;
enum socket_type sock_type;
uint32_t payload, bandwidth, duration, wait_seconds;
uint16_t parallel_streams;

int main(int argc, char **argv){

	

	while ((opt = getopt(argc, argv, "apifsclbntdw:" )) != -1 ){
		switch(opt) {
		case 'a': /* address to bind or to send */
			address = strdup(optarg);
			break;
		case 'p': /* listening port of the TCP channel */
			port = strdup(optarg);
			break;
		case 'i': /* Interval in seconds to print information */
			interval_seconds = atoi(optarg);
			break;
		case 'f': /* file to store the input */
			filename = strdup(optarg);
			break;
		case 's': /* Act as a server 			*/
			sock_type = server;
			break;
		case 'c': /* Act as a client */
			sock_type = client;
			break;
		case 'l': /* UDP payload size in bytes */
			payload = atoi(optarg);
			break;
		case 'b': /* Bandwidth in bits per second of the data stream */
			bandwidth = atoi(optarg);
			break;
		case 'n': /* number of parallel data streams */
			parallel_streams = atoi(optarg);
			break;
		case 't': /* Experiment duration in seconds */
			duration = atoi(optarg);
			break;	
		case 'd': /* Measure the one way delay */
			one_way_delay = 1;
			break;
		case 'w': /* Wait seconds before starting the data transmission */
			wait_seconds = atoi(optarg);
			break;
		}

	}	


}
