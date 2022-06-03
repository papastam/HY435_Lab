#include "mini_iperf.h"
#include <pthread.h>

enum socket_type sock_type_sd;

int udp_fd;
int tcp_fd;

int *udp_fd_list;
int *tcp_fd_list;

void sig_handler(){
	//clean_up(tcp_fd);
	exit(1);
}

void setUp_required_options( experiment_options_t *input_options){
	if(input_options->interval_duration == 0 )
		input_options->interval_duration = 1;

	if(input_options->packet_length == 0 )
		input_options->packet_length = 1024 * 8;

	if(input_options->bandwidth == 0 )
		input_options->bandwidth = 0;

	if(input_options->experiment_duration == 0)
		input_options->experiment_duration = 10;


	if(input_options->packet_length == 0)
		input_options->packet_length = 1024;	


	input_options->offset = 0;

	udp_fd_list = malloc((input_options->parallel_streams + 1) * sizeof(int));
	tcp_fd_list = malloc((input_options->parallel_streams + 1) * sizeof(int));
	
}

int main(int argc, char **argv){

	char *bandwidth_str = NULL;
	int opt, outfile_fd, metric;
	experiment_options_t *exp_options;
	pthread_t *threadsarr;
	signal(SIGINT, sig_handler);
	exp_options = malloc(sizeof(experiment_options_t));

	memset(exp_options, 0, sizeof(exp_options));	
	exp_options->address 			= DEFAULT_ADDRESS;
	exp_options->port 				= DEFAULT_UDP_PORT;
	sock_type_sd  					= UNDEFINED;
	exp_options->packet_length 		= 0;
	while ((opt = getopt(argc, argv, "scda:p:i:f:l:b:n:t:w:" )) != -1 ){
		switch(opt) {
		case 's':
			/* If -s is set program runs on server mode */
			exp_options->sock_type = SERVER;
			sock_type_sd = SERVER;
			break;
		case 'c':
			/* If -c is set program runs on client mode */ 
			exp_options->sock_type = CLIENT;
			sock_type_sd = CLIENT;
			break;
		case 'd':  //CLINET ONLY
			/*If -d is set measure the one way delay */
			if(exp_options->sock_type==CLIENT)
				exp_options->one_way_delay = 1;
			else
				printf("One way delay (specified with -d) can be measured on a client iperf instance only");
			break;
		case 'a': /* address to bind or to send */
			exp_options->address = strdup(optarg);
			// printf("Address is %s\n", exp_options.address);
			break;
		case 'p': /* listening port of the TCP channel */
			exp_options->port = atoi(optarg);
			// printf("port is %d\n", exp_options.port);
			break;
		case 'i': /* Interval in seconds to print information */
			exp_options->interval_duration = atoi(optarg);
			// printf("Interval is %d\n", exp_options.interval_duration);
			break;
		case 'f': /* file to store the input */
			exp_options->filename = strdup(optarg);
			// printf("File to store is %s\n", exp_options.filename);
			outfile_fd = open(exp_options->filename,O_CREAT | O_WRONLY | O_TRUNC,0644);
			dup2(outfile_fd, 1); 	
			break;
		case 'l': /* UDP payload size in bytes */ //CLINET ONLY
			if(exp_options->sock_type==CLIENT)
				exp_options->packet_length = atoi(optarg);
			else
				printf("Payload size (specified with -l) can be defined on a client iperf instance only");
			break;
		case 'b': /* Bandwidth in bits per second of the data stream */ //CLINET ONLY
			if(exp_options->sock_type==CLIENT){
				bandwidth_str = strdup(optarg);	
				metric = getMetric(bandwidth_str);
				uint32_t bps = convertToBps(atoi(optarg), metric);
				printf("Bps is %u\n", bps);
				exp_options->bandwidth = bps;
				free(bandwidth_str);
			}else
				printf("Bandwidth (specified with -b) can be defined on a client iperf instance only");
			break;
		case 'n': /* number of parallel data streams */ //CLINET ONLY
			if(exp_options->sock_type==CLIENT){
				exp_options->parallel_streams = atoi(optarg);
			}else
				printf("Parallel streams (specified with -n) can be used on a client iperf instance only");
			break;
		case 't': /* Experiment duration in seconds */ //CLINET ONLY
			if(exp_options->sock_type==CLIENT)
				exp_options->experiment_duration = atoi(optarg);
			else
				printf("Experiment duration (specified with -t) can be defined on a client iperf instance only");
			break;	
		case 'w': /* Wait seconds before starting the data transmission */ //CLINET ONLY
			if(exp_options->sock_type==CLIENT)
				exp_options->wait_duration = atoi(optarg);
			else
				printf("Wait seconds (specified with -w) can be defined on a client iperf instance only");
			break;
		default:
			printf("Usage for server mini_iperf -a [address] -p [port] -i" 
					" [interval] -f [filename] -s .\n" 
					"Usage for client mini_iperf -a [address] -p [port] \n" 
					"-i [interval] -f [filename] -c [act as client] \n"
					"-l [payload in bytes] -b [bandwidth per sec's] \n"
					"-t [experiment duration in seconds] \n"
					"-d [measure one way delay] \n"
					"-w [wait duration in seconds] \n");
			exit(EXIT_FAILURE);
		}
	}	

	//Handle insuffisient arguement cases
	if(exp_options->sock_type==UNDEFINED){
		close(outfile_fd);
		printf("Please specify server or client usage\nServer: '-s'\nClient: '-c'\n");
		exit(-1);
	}

	setUp_required_options(exp_options);
	if(exp_options->parallel_streams > 1){
		threadsarr 	= malloc(exp_options->parallel_streams * sizeof(pthread_t));
	}
	
	//lifecycle of out mini iperf as a server or a client
	if(exp_options->sock_type==SERVER){
		conduct_experiment_master_server(exp_options);
	}else if(exp_options->sock_type==CLIENT){
		exp_options->NTPsocket = initNTPconnection(0);
		
		if(exp_options->parallel_streams<=1){
			conduct_experiment_client(exp_options);	
		}else if(exp_options->parallel_streams>1){
			conduct_experiment_client_parallel(threadsarr, exp_options); 
		}
	}
	
	free(tcp_fd_list);
	free(udp_fd_list);
	if(exp_options->parallel_streams > 1)
		free(threadsarr);

	free(exp_options);
	return 0;
}
