#include "mini_iperf.h"
#include <assert.h>
#include <errno.h>

void die(char *err_msg, int offset){
	if(offset)
		printf("Thread %d requested unexpected program termination!!!!!\n",offset);
	perror(err_msg);
	clean_up(tcp_fd_list[offset], udp_fd_list[offset]);
	fflush(stdout);
	exit(EXIT_FAILURE);
}

void printTCPheader(tcp_header_t *header){
	printf("\n-------------TCP HEADER------------\n");
	printf("Packet length :\t\t %d\n",header->packet_length);
	printf("# of parallel streams :\t %d\n",header->parallel_streams);
	printf("Bandwidth :\t\t %d\n",header->bandwidth);
	printf("Duration (sec) :\t %d\n",header->experiment_duration);
	printf("Wait duration (sec) :\t %d\n",header->wait_duration);
	printf("Port :\t\t\t %d\n",header->port);
	printf("Interval duration :\t %d\n",header->interval_duration);
	printf("-----------------------------------\n\n");
}

void printOptions(experiment_options_t *exp_options, char* who){
	printf("\n--------------%s OPTIONS--------------\n",who);
	printf("Packet length :\t\t %d\n",exp_options->packet_length);
	printf("# of parallel streams :\t %d\n",exp_options->parallel_streams);
	printf("Bandwidth :\t\t %d\n",exp_options->bandwidth);
	printf("Duration (sec) :\t %d\n",exp_options->experiment_duration);
	printf("Wait duration (sec) :\t %d\n",exp_options->wait_duration);
	printf("Port :\t\t\t %d\n",exp_options->port);
	printf("Interval duration :\t %d\n",exp_options->interval_duration);
	printf("-----------------------------------\n\n");
}

/**
 * @brief initializes both of the iperf communication channels ot the client
 * 
 * @param address 
 * @param pstreams 
 * @param bandwidth 
 * @param expert_duration 
 * @param wait_duration 
 * @param port 
 * @param interval_duration 
 * @return -1 if the initialization failed, 1 if it succedded
 */
int init_iperf_client(const experiment_options_t  *exp_options){
    
   	struct sockaddr_in serveraddr;    
   	tcp_header_t initheader;
	uint32_t iaddr;
	uint16_t offset;
	uint16_t port;

	offset = exp_options->offset;

    initheader.port                 = htons(exp_options->port) + offset;
    initheader.experiment_duration  = htons(exp_options->experiment_duration);
    initheader.parallel_streams     = htons(exp_options->parallel_streams);
    initheader.interval_duration    = htons(exp_options->interval_duration);
    initheader.packet_length 		= ntohl(exp_options->packet_length);
    initheader.bandwidth            = htonl(exp_options->bandwidth);
    initheader.wait_duration        = htons(exp_options->wait_duration);
	initheader.offset				= htons(offset);
	initheader.one_way_delay		= htons(exp_options->one_way_delay);
	                                                                            
	
	// check return codes from socket, inet_pton
    tcp_fd_list[offset] = socket(AF_INET, SOCK_STREAM, 0);
    inet_pton(AF_INET, exp_options->address, &iaddr);

	port = DEFAULT_TCP_PORT + offset;		
	serveraddr.sin_addr.s_addr 	= iaddr;
    serveraddr.sin_family 		= AF_INET;
    serveraddr.sin_port 		= htons(port);//+ offset if exists

	printf("Connecting to server %s:%u\n",exp_options->address, port);
    // connect the client socket to server socket
    if (connect(tcp_fd_list[offset], (struct sockaddr *)&serveraddr,
			sizeof(serveraddr)) != 0) {
        die("connection with the TCP server failed...\n", offset);
    }

    if(send(tcp_fd_list[offset],(void *) &initheader, 
			sizeof(initheader), 0) < 0){
        die("TCP initialization header failed to send...\n", offset);
    }

	return 1;
}

/**
 * @brief terminates both of the iperf communication channels ot the client
 * 
 * @return int 
 */
int terminate_iperf_client(int tcp_fd, int offset){
	tcp_header_t terminate_header;
	
	terminate_header.termination_signal=htons( TERMINATE_EXP);
	printf("Termintating the connection\n");

	if(send(tcp_fd_list[offset],(void*) &terminate_header,sizeof(tcp_header_t), 0) < 0){
		die("Termination send unsuccesfull\n", offset);
	}


	clean_up(tcp_fd_list[offset], udp_fd_list[offset]);
	return 1;
}


/*Creates and returns udp socket descriptor 	*/
int get_udp_socket(uint16_t offset){
	
	int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);	
	if(udp_socket == -1 ){
		close(udp_socket);
		die("udp init socket : ", offset);
	}
	
	return udp_socket;	

}



void* append_header(void *buff, size_t length, udp_header_t header) {
	
	header.checksum = crc32(buff, length );
	
	memcpy(buff, &header, sizeof(udp_header_t));
	memcpy(buff + sizeof(udp_header_t) , buff, length - sizeof(udp_header_t) ); //SEG


	return buff;
}


uint64_t timespec_diff_lowres(const struct timespec *timeA, const struct timespec *timeB){
	return timeA->tv_sec - timeB->tv_sec;
}


uint64_t timespec_diff_highres(const struct timespec *timeA, const struct timespec *timeB){
	return ((timeA->tv_sec * 1000000000) + timeA->tv_nsec) - 
				((timeB->tv_sec * 1000000000) + timeB->tv_nsec);
}

int detect_secchange(uint64_t time, uint16_t currsec){
	// printf("time: %ld, currsec:%d, time/10^9:%ld\n",time,currsec,time/1000000000U);
	if(time/1000000000U != currsec)
		return 1;
	else
		return 0;
}

long remainingnsec(uint64_t time){
	return 1000000000U - (time - ((time/1000000000U)*1000000000U));
}

uint calculateThroughput(uint64_t interval, ssize_t bytessent){
	double secondsinterval = interval/1000000000.0;
	ssize_t bitssent = bytessent * 8;
	// printf("time: %f, bytes sent: %ld --> CT: %f\n",secondsinterval, bitssent, bitssent / secondsinterval);
	return bitssent / secondsinterval;
}

ssize_t send_wbandwidth(bandwidthControl_t *controlstr, int offset){
	ssize_t ret;
	struct timespec now, sleeptime_timespec;
	uint64_t diff;
	uint current_throughput;
	udp_header_t header;
	void *data, *packet;

	if(controlstr->payload_len < sizeof(udp_header_t)){
		controlstr->payload_len = 8;
	}
	data = malloc(controlstr->payload_len + sizeof(udp_header_t));
	
	if(!data){
		die("bandwidth send malloc error ", offset);
	}

	controlstr->packet_id = controlstr->packet_id + 1;
	header.packet_id = htonl(controlstr->packet_id);
	packet = append_header(data, controlstr->payload_len + sizeof(udp_header_t),
				header);	

	ret = sendto(udp_fd_list[offset], data, controlstr->payload_len, 0, (struct sockaddr *)&controlstr->sa, sizeof(controlstr->sa) ); 
	if(ret == -1 ){
 		die("error start_experiment sendto : ", offset);
	}
	ret -= sizeof(udp_header_t);
	clock_gettime(CLOCK_MONOTONIC, &now);

	if(controlstr->bandwidth>0){
		diff = timespec_diff_highres(&now, &controlstr->bandwidthEnd);
		clock_gettime(CLOCK_MONOTONIC, &controlstr->bandwidthEnd);
		current_throughput = calculateThroughput(diff, ret);
		// printf("CT: %d, BW: %d\n", current_throughput, controlstr->bandwidth);
		if(current_throughput != controlstr->bandwidth){
			if(current_throughput > controlstr->bandwidth){
				controlstr->sleeptime = 10U + controlstr->sleeptime * 1.2;
			}else{
				controlstr->sleeptime = controlstr->sleeptime * 0.8;
			}
			// printf("Sleeping for %u ms\n\n",controlstr->sleeptime);
			sleeptime_timespec.tv_nsec= controlstr->sleeptime;
			sleeptime_timespec.tv_sec = 0;

			nanosleep(&sleeptime_timespec, NULL);
		}		
	}
	
	//FIRST APPROACH
	
	// if(detect_secchange(diff, *currsec)){//if a second has passed since the last reset
	// 	(*bytesthissec)=0;//reset bytesthissec 
	// 	(*currsec)++;// and update currsec
	// }else{
	// 	(*bytesthissec)+=ret;//add the new sent bytes
	// 	if((*bytesthissec) >= bandwidth/8){ //if more bytes are sed this second than the desired bandwidth
	// 		// printf("bytes sent: %d (%d bits), bandwidth of %dbps (%dBps)\n", *bytesthissec, (*bytesthissec)*8, bandwidth, bandwidth/8);
	// 		sleeptime.tv_nsec = remainingnsec(diff); //create the sleeptime struct
	// 		sleeptime.tv_sec  = 0;
	// 		// printf("sleeping for %ld ns\n\n",sleeptime.tv_nsec);
	// 		// printf("elapsed:%ld, remaining:%ld, elapsed+remaining:%ld\n\n",diff,sleeptime.tv_nsec,diff+sleeptime.tv_nsec);
	// 		nanosleep(&sleeptime, NULL);//and sleep
	// 	}
	// }
	
	return ret;
}

void initBandwidthControl(const experiment_options_t *exp_options, bandwidthControl_t *bandwidthCtrl, struct sockaddr_in sa, 
	uint32_t payload_len){
	
	memset(bandwidthCtrl, 0, sizeof(bandwidthControl_t));
	bandwidthCtrl->bandwidth = exp_options->bandwidth;
	bandwidthCtrl->sa = sa;
	bandwidthCtrl->payload_len = payload_len;	

}

int measure_one_way_delay_client(int sock,const experiment_options_t *exp_options){
	NTPmeasurement_t *measurement;
	struct sockaddr_in sa;
	size_t packet_length;
	int ret, lastsend=0;
	uint64_t time_elapsed;
	int offset; 	
	const uint64_t duration = exp_options->experiment_duration;
	bandwidthControl_t bandwidthCtrl;
	int file_fd;
	struct timespec start, end;

	offset = exp_options->offset;

	if(offset > 1){
		char filename[30];
		sprintf(filename, "(client_threads)-thread#%d.txt",offset);
		file_fd = open(filename,O_CREAT | O_WRONLY | O_TRUNC,0644);
	}

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(exp_options->port + offset);
	inet_pton(AF_INET, exp_options->address, &(sa.sin_addr));
	packet_length = exp_options->packet_length + sizeof(udp_header_t);

	measurement = malloc(sizeof(NTPmeasurement_t));
	measurement->arrival_time = getTimeFromNTP(exp_options->NTPsocket,offset);

	ret = sendto(sock, (void*) measurement, packet_length, 0, (struct sockaddr *)&sa, sizeof(sa) ); 
	clock_gettime(CLOCK_MONOTONIC, &start);

	for(;;){	
		// checkforstats(offset, file_fd);

		measurement = malloc(sizeof(NTPmeasurement_t));
		measurement->arrival_time = getTimeFromNTP(exp_options->NTPsocket,offset);

		ret = sendto(sock, (void*) measurement, packet_length, 0, (struct sockaddr *)&sa, sizeof(sa) ); 
		
		clock_gettime(CLOCK_MONOTONIC, &end);
		if(ret == -1 ){
			die("error start_experiment sendto : ", offset);
		}
		time_elapsed = timespec_diff_lowres(&end, &start );

		if(lastsend)break;

		if(time_elapsed >= duration )lastsend=1;		
	}

	return 1;
}

statistics_t* checkforstats(int offset, int file_fd){
	struct pollfd tcp_poll_fd[1];
	statistics_t stats;
	int ret;

	memset(tcp_poll_fd,0,sizeof(tcp_poll_fd));
	tcp_poll_fd->fd=tcp_fd_list[offset];
	tcp_poll_fd->events=POLL_IN;

	poll(tcp_poll_fd, 1, 0);

	if(!(tcp_poll_fd->revents & POLL_IN)){
		return NULL;
	}

	if(recv(tcp_fd_list[offset],(void*) &stats, sizeof(statistics_t), 0) > 0){
		stats.socket_id					 = ntohl(stats.socket_id);
		stats.throughput				 = ntohl(stats.throughput);
		stats.goodput					 = ntohl(stats.goodput);
		stats.pct_loss_percentage		 = ntohl(stats.pct_loss_percentage);
		stats.jitter					 = ntohl(stats.jitter);
		stats.jitter_standard_deviation	 = ntohl(stats.jitter_standard_deviation);
		stats.one_way_delay				 = ntohl(stats.one_way_delay);
		// stats.time						 = ntohl(stats.time);
		stats.sec_min					 = ntohl(stats.sec_min);
		stats.sec_max					 = ntohl(stats.sec_max);

		printStatisticsPerSeconds(&stats,offset,file_fd);
	}else{
		// printf("Statistics reception failed!\n");
	}
}


uint32_t modify_packet_len(const experiment_options_t *exp_options){
	uint32_t payload = exp_options->packet_length;
	uint32_t bandwidth_bytes = exp_options->bandwidth / 8;
	if(bandwidth_bytes/payload < 10000000){
		payload = bandwidth_bytes / 10000000;
	}
	return payload;
}

int start_experiment_client(int sock, const experiment_options_t* exp_options){ 
	char *data;
	struct sockaddr_in sa;
	size_t packet_length;
	struct timespec start, end;
	int ret, lastsend=0;
	uint64_t time_elapsed;
	int offset; 	
	const uint64_t duration = exp_options->experiment_duration;
	bandwidthControl_t bandwidthCtrl;
	int file_fd;

	offset = exp_options->offset;

	if(offset > 1){
		char filename[30];
		sprintf(filename, "(client_threads)-thread#%d.txt",offset);
		file_fd = open(filename,O_CREAT | O_WRONLY | O_TRUNC,0644);
	}

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(exp_options->port + offset);
	inet_pton(AF_INET, exp_options->address, &(sa.sin_addr));
	packet_length = exp_options->packet_length + sizeof(udp_header_t);
	data = malloc(packet_length) ;
		
	initBandwidthControl(exp_options,&bandwidthCtrl, sa, packet_length);

	ret = sendto(sock, data, packet_length, 0, (struct sockaddr *)&sa, sizeof(sa) ); 
	clock_gettime(CLOCK_MONOTONIC, &start);

	for(;;){	
		checkforstats(offset, file_fd);

		ret = send_wbandwidth(&bandwidthCtrl, offset);
		
		clock_gettime(CLOCK_MONOTONIC, &end);
		if(ret == -1 ){
			die("error start_experiment sendto : ", offset);
		}
		time_elapsed = timespec_diff_lowres(&end, &start );

		if(lastsend)break;

		if(time_elapsed >= duration )lastsend=1;
		
	}
	
	free(data);

	return 1;
}

void free_threads(experiment_options_t **thr_opt, experiment_options_t *exp){
	
	for(int i = 0; i < exp->parallel_streams; i++){
		free(thr_opt[i]);
	}
}

int conduct_experiment_client_parallel(pthread_t *threads_arr, const experiment_options_t *exp_options){
	
	char b[100];

	if(init_iperf_client(exp_options) < 0 ){
		die("error init iperf client on parallel", 0);
	}

	if( recv(tcp_fd_list[0], b, 100, 0) < 0){
		die("error receive client parallel ", 0);
	}

	for(int i = 0; i < exp_options->parallel_streams; i++){
		experiment_options_t *thread_options;
		thread_options = malloc(sizeof(experiment_options_t));
		if(!thread_options){
			die("malloc conduct parallel client error : ", 0);
		}
		memcpy(thread_options, exp_options, sizeof(experiment_options_t));
		thread_options->offset = i + 1;
		thread_options->parallel_streams = 1;
		thread_options->isThread = 1;		

		pthread_create(&threads_arr[i], NULL, &start_thread_exp, 
			thread_options);
		
	}	
	for(int i = 0; i < exp_options->parallel_streams; i++){
		pthread_join(threads_arr[i], NULL);	
	}
}

void * start_thread_exp(void* exp_opt){
	experiment_options_t *exp_options = (experiment_options_t *)exp_opt;

	// printf("offset:%d\n",exp_options->offset);
	if(exp_options->sock_type==SERVER){
		
		conduct_experiment_server(exp_options);
	
	}else if(exp_options->sock_type==CLIENT){
		conduct_experiment_client(exp_options);
	}

	pthread_exit(NULL);
}

int conduct_experiment_client(const experiment_options_t *input_options){
	
 	int udp_sock, tcp_fd, offset;
	init_iperf_client(input_options);
	offset = input_options->offset;
	tcp_fd = tcp_fd_list[offset];

	udp_sock = get_udp_socket(offset);
	udp_fd_list[offset] = udp_sock;	
	
	if(input_options->one_way_delay)
		measure_one_way_delay_client(udp_sock,input_options);
	else
		start_experiment_client(udp_sock, input_options);
	
	terminate_iperf_client(tcp_fd_list[offset], offset);
	return 1;
}

void clean_up(int tcp_fd, int udp_fd){
	close(tcp_fd);
	close(udp_fd);
}
