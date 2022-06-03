#include "../inc/mini_iperf.h"

void *start_experiment_asThread(void* exp_options){
	conduct_experiment_server((experiment_options_t*) exp_options);
}

void init_multiple_iperf_streams(const experiment_options_t *exp_options,
		pthread_t *threadsarr){

	char b[100];
	
	
	for(int i=0; i < exp_options->parallel_streams; i++){
		experiment_options_t *thread_options = malloc(sizeof(experiment_options_t ));
		memcpy(thread_options, exp_options, sizeof(experiment_options_t));
		thread_options->isThread = 1;
		thread_options->offset = i + 1;
		thread_options->parallel_streams = 1;
		// printf("thread offset is %u\n", thread_options[i]->offset);
		fflush(stdout);
		pthread_create(&threadsarr[i],NULL,start_experiment_asThread,thread_options);
	}

	send(tcp_fd_list[0], b, 100, 0);
	

	for(int i = 0; i < exp_options->parallel_streams; i++){
		pthread_join(threadsarr[i],NULL);
	}
	
}

/**
 *  @brief initializes both of the iperf communication channels ot the server
 *
 *  @param address the address for the server to bind
 *  @param port the port for the server to bind
 * 
 *  @return experiment_options_t that the socket recieved from the connecting client 
 */
experiment_options_t* init_iperf_server(const experiment_options_t *exp_options){
  	tcp_header_t recieveheader, initheader;
	experiment_options_t *recieved_options;
   	struct sockaddr_in serveraddr, clientaddress;
	socklen_t socklen;
	uint32_t iaddr;
    int rc, on = 1;
	int tcp_fd;
	uint16_t offset, port;

	offset = exp_options->offset;
	port = DEFAULT_TCP_PORT + offset;

   	tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_fd == -1) {
		close(tcp_fd);
		exit(-1);
	}
	
	rc = setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR,
			 	(char *)&on, sizeof(on));
	if(rc < 0){
		close(tcp_fd);
		exit(-1);
	}

    inet_pton(AF_INET, exp_options->address, &iaddr);
	serveraddr.sin_addr.s_addr 	= iaddr;
    serveraddr.sin_family 		= AF_INET;
    serveraddr.sin_port 		= htons(port);

	socklen = sizeof(serveraddr);
    //Bind the socket to the givven address
    if((bind(tcp_fd, (struct sockaddr *)&serveraddr, 
								socklen )) != 0){
   	close(tcp_fd);
		exit(-1); 
	}

    //Listen for incoming connections
    if ((listen(tcp_fd, 5)) != 0) {
   	close(tcp_fd);
		exit(-1); 
	}

	printf("Listening to: %s:%u\n",exp_options->address, port);
  
	socklen = sizeof(clientaddress);
    // Accept the data packet from client and verification
    tcp_fd= accept(tcp_fd, (struct sockaddr *)&clientaddress, &socklen);
    if (tcp_fd < 0) {
   	close(tcp_fd);
		exit(-1); 
	}

    if(recv(tcp_fd, (void *) &recieveheader, sizeof(recieveheader), 0) < 0){
   	close(tcp_fd);
		exit(-1); 
	}

	recieved_options = malloc(sizeof(experiment_options_t ));

	recieveheader.port 					= recieved_options->port 				= ntohs(recieveheader.port);
	recieveheader.experiment_duration 	= recieved_options->experiment_duration = ntohs(recieveheader.experiment_duration);
    recieveheader.parallel_streams 		= recieved_options->parallel_streams 	= ntohs(recieveheader.parallel_streams);
    recieveheader.packet_length 		= recieved_options->packet_length 		= ntohl(recieveheader.packet_length);
    recieveheader.bandwidth 			= recieved_options->bandwidth 			= ntohl(recieveheader.bandwidth);
    recieveheader.wait_duration 		= recieved_options->wait_duration 		= ntohs(recieveheader.wait_duration);
    recieveheader.offset 				= recieved_options->offset 				= ntohs(recieveheader.offset);
    recieveheader.one_way_delay			= recieved_options->one_way_delay		= ntohs(recieveheader.one_way_delay);
	recieved_options->interval_duration = exp_options->interval_duration;
    recieved_options->address 			= exp_options->address;
    recieved_options->filename 			= exp_options->filename;
	recieved_options->sock_type			= SERVER;
    
	tcp_fd_list[recieved_options->offset] = tcp_fd;
	return recieved_options;
}

int initNTPconnection(int offset){
	struct sockaddr_in ntp_server_addr;
	struct hostent *ntp_server;
	const char *host_name = "us.pool.ntp.org";
	int portn = 123;
	
	socklen_t socketlen;
	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(fd < 0) {
		close(fd);	
		die("error opening socket calculate onewaydelay ", offset);
	}
	ntp_server = gethostbyname(host_name);
	if(!ntp_server) {
		close(fd);
		die("error no such host ", offset);
	}
	bzero( ( char* ) &ntp_server_addr, sizeof( ntp_server_addr ) );
	ntp_server_addr.sin_family = AF_INET;
	bcopy( ( char* )ntp_server->h_addr, ( char* ) &ntp_server_addr.sin_addr.s_addr, ntp_server->h_length );

	ntp_server_addr.sin_port = htons(portn);

	if( connect( fd, (struct sockaddr *)&ntp_server_addr, sizeof(ntp_server_addr) ) < 0){
		close(fd);
		die("connecting ntp server error ", offset);
	}
	
	return fd;	
}

time_t getTimeFromNTP(int NTPsocket,int offset){
	int ret;
	ntp_packet_t packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  	memset( &packet, 0, sizeof( packet ) );
	*( (char *)&packet + 0) = 0x1b; /* 27 in base ten 000011011 in base 2 */

	ret = write(NTPsocket, (char *)&packet, sizeof(ntp_packet_t));
	if(ret < 0 ){
		die("error sending to ntp server ", offset);
	}

	ret = read(NTPsocket, (char *)&packet, sizeof(ntp_packet_t));
	if(ret < 0){
		die("error reading from ntp server ", offset);
	}

	packet.txTm_s = ntohl(packet.txTm_s); 
	packet.txTm_f = ntohl(packet.txTm_f);
	time_t txTm = ( time_t ) ( packet.txTm_s - NTP_TIMESTAMP_DELTA );	
	return txTm;

	// printf("time from ntp is %u, %u\n", packet.txTm_s, packet.txTm_f);
}

/**
 * @brief terminates both of the iperf communication channels ot the server
 * 
 * @return int 
 */
int terminate_iperf_server(const experiment_options_t *exp_options){
    
	int offset = exp_options->offset;
	clean_up(tcp_fd_list[offset], udp_fd_list[offset]);
	exit(EXIT_SUCCESS);
}

ssize_t send_stats(const statistics_t *stats, int offset){
	statistics_t sendstats;
	ssize_t ret;

	sendstats.socket_id					 = htonl(stats->socket_id); 	
	sendstats.throughput				 = htonl(stats->throughput); 	
	sendstats.goodput					 = htonl(stats->goodput); 	
	sendstats.pct_loss_percentage		 = htonl(stats->pct_loss_percentage); 	
	sendstats.jitter					 = htonl(stats->jitter); 	
	sendstats.jitter_standard_deviation	 = htonl(stats->jitter_standard_deviation); 	
	sendstats.one_way_delay				 = htonl(stats->one_way_delay); 	
	// sendstats.time
	sendstats.sec_min					 = htonl(stats->sec_min); 	
	sendstats.sec_max					 = htonl(stats->sec_max); 	

	if((ret = send(tcp_fd_list[offset], (void*) &sendstats, sizeof(statistics_t),0)) < 0){
		die("Error sending statistics to the client", offset);
	}

	return ret;
}

/* param received data received in user defined seconds
	// TODO NEEDS REFACTORING
*/
void printStatisticsPerSeconds(const statistics_t *statistics, uint16_t offset, int file_fd){
	double megabytes = 8 * (statistics->throughput /  (1024.0 * 1024.0));
	double goodput_mbs = 8 * (statistics->goodput / (1024.0 * 1024.0));
	uint64_t jitter_ms = statistics->jitter / (1000) ;
	
	if(offset <= 1){
		printf("%d-%d secs Throughput : %2.2fMbs  Goodput : %2.2fMbs\
 Jitter : %lums JitterDeviation : %lums  pct_loss_percentage : %2.2f\n",
			statistics->sec_min, statistics->sec_max, 
			megabytes, goodput_mbs, statistics->jitter, statistics->jitter_standard_deviation, 
			statistics->pct_loss_percentage);	
	}else{
		dprintf(file_fd,"%d-%d secs Throughput : %2.2fMbs  Goodput : %2.2fMbs\
 Jitter : %lums JitterDeviation : %lums  pct_loss_percentage : %2.2f\n",
			statistics->sec_min, statistics->sec_max, 
			megabytes, goodput_mbs, statistics->jitter, statistics->jitter_standard_deviation, 
			statistics->pct_loss_percentage);	
	}

}

void calculateStatistics(statistics_t *stats,
const experiment_options_t *exp_options, int total_packets){

	uint16_t tmp_max = stats->sec_max;
	
 	stats->sec_max += exp_options->interval_duration;
 	stats->sec_min = tmp_max == 0 ? 0 : tmp_max;
}


uint64_t calculateJitter(const struct timespec *t1_tp, 
						const struct timespec *t2_tp, 
						const struct timespec *t3_tp){

	uint64_t d0;
	uint64_t d1;

	d0 = timespec_diff_highres(t1_tp, t2_tp);	
	d1 = timespec_diff_highres(t2_tp, t3_tp);

	if(d1 > d0)
		return d1 - d0;
	else
		return d0 - d1;	
	
}

uint64_t jitter_to_ms(uint64_t jitter){

	return (jitter / 1000);

}


uint64_t updateJitterTimes(struct timespec *t1, struct timespec *t2, struct timespec *t3){

	if(t1->tv_nsec == 0 && t2->tv_nsec == 0 && t3->tv_nsec == 0){
		clock_gettime(CLOCK_MONOTONIC, t1);
	}else if(t1->tv_nsec != 0 && t2->tv_nsec == 0 && t3->tv_nsec == 0){
		clock_gettime(CLOCK_MONOTONIC, t2);
	}else if(t1->tv_nsec != 0 && t2->tv_nsec != 0 && t3->tv_nsec == 0){
		clock_gettime(CLOCK_MONOTONIC, t3);
	}else{
		memcpy(t1, t2, sizeof(struct timespec));
		memcpy(t2, t3, sizeof(struct timespec));
		clock_gettime(CLOCK_MONOTONIC, t3);
	}

}

double calculateLostPackets(uint32_t lost_packets, uint32_t total_packets){
	return ((lost_packets * 100) / total_packets);
}


void initJittersArray(jitter_deviation_t *jitters){
	jitters->sizeLimit = 100000;
	jitters->fillSize = 0;
	jitters->jitters_arr = malloc(sizeof(uint32_t) * jitters->sizeLimit);
}

void updateJittersArray(jitter_deviation_t *jitters, uint32_t new_jitter) {

	uint32_t *newJitters;
	if(jitters->sizeLimit  == jitters->fillSize){
		jitters->sizeLimit  += jitters->sizeLimit;
		newJitters = realloc(jitters->jitters_arr,
			jitters->sizeLimit * sizeof(uint32_t));
		jitters->jitters_arr = newJitters;
	}
	jitters->jitters_arr[jitters->fillSize++] = new_jitter;
}

void clearJittersArray(jitter_deviation_t *jitters){
	jitters->sizeLimit = 100000;
	jitters->fillSize = 0;
}

uint64_t getJitterStandardDeviation(jitter_deviation_t *jitters ){
	uint64_t i = 0;
	uint32_t mean_jitter;
	uint64_t sum_of_jitters = 0U;
	uint64_t sum_of_variances = 0U;
	uint64_t jitterStandardDeviation;

	if(jitters->fillSize == 0)
		return 0;

	for(i = 0; i < jitters->fillSize; i++){
		uint32_t jitter = jitters->jitters_arr[i];
		sum_of_jitters += jitter;
	}
		mean_jitter = sum_of_jitters / (jitters->fillSize);
	for(i = 0; i < jitters->fillSize; i++){
		uint32_t jitter = jitters->jitters_arr[i];
		uint64_t variance = (jitter - mean_jitter) * (jitter - mean_jitter);
		sum_of_variances = sum_of_variances + variance;
	}
	sum_of_variances = sum_of_variances / (jitters->fillSize) ;
	jitterStandardDeviation = sqrt(sum_of_variances);
	return jitterStandardDeviation;

}

void freeJittersArray(jitter_deviation_t *jitters){
	free(jitters->jitters_arr);
}

int checkForTermination(int offset){
	struct pollfd tcp_poll_fd[1];
	tcp_header_t terminate_header;
	int ready;

	memset(tcp_poll_fd,0,sizeof(tcp_poll_fd));
	tcp_poll_fd->fd=tcp_fd_list[offset];
	tcp_poll_fd->events=POLL_IN;

	poll(tcp_poll_fd, 1, 0);

	if(!(tcp_poll_fd->revents & POLL_IN)){
		return 0;
	}
	if(recv(tcp_fd_list[offset],(void*) &terminate_header, sizeof(tcp_header_t), 0) > 0){
		if(ntohs(terminate_header.termination_signal)==TERMINATE_EXP){
			printf("Termination requested! Terminating...\n");	
		
			return TERMINATE_EXP;
		}
	}
	
	return 0;
}

long calculateOWD(const experiment_options_t *exp_options, NTPmeasurement_t *measurement){
	int offset = exp_options->offset;
	time_t start_time;

	start_time = getTimeFromNTP(exp_options->NTPsocket,offset);
	// printf("(%ld-%ld =) %ld\n",start_time,measurement->arrival_time,start_time - measurement->arrival_time);
	return start_time - measurement->arrival_time;
}

int measure_one_way_delay_server(int sock,const experiment_options_t* exp_options){
	int packet_len, addr_len, len;
	int offset = exp_options->offset;
	char *addr = NULL;
	struct sockaddr_in sa;
	int file_fd;
	NTPmeasurement_t *measurement;
	long owd;
	struct timespec start, end, t1, t2;
	double estimatedRtt = 0;
	uint64_t prevtime=0U, time_elapsed;
	int i = 0;

	if(offset > 1){
		char filename[50];
		sprintf(filename, "(server_threads)-thread#%d.txt",((experiment_options_t *)exp_options)->offset);
		file_fd = open(filename,O_CREAT | O_WRONLY | O_TRUNC,0644);
	}

	packet_len = exp_options->packet_length + sizeof(NTPmeasurement_t);
	addr = exp_options->address;
	
	sa.sin_family = AF_INET;	
	sa.sin_port = htons(exp_options->port + offset);
	if(!addr){
		sa.sin_addr.s_addr = INADDR_ANY; 
	}else{
		int ret = inet_pton(AF_INET, addr, &(sa.sin_addr));
		if(ret == 0 ){
			die("Address to bind from server is not valid ", offset);
		}
	}

	if(bind(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0 ){
		die("server bind", offset);
	}

	addr_len = sizeof(sa);
	/* Discard first packet */
	len = recvfrom(sock,(void*) measurement, packet_len, 0, (struct sockaddr *)&sa,
 		&addr_len);

	if(len < 0 ){
		die("error first receive ", offset);
	}
	
	memset(&start, 0, sizeof(start));
	memset(&end, 0, sizeof(end));

	clock_gettime(CLOCK_MONOTONIC, &start);
	for(;;){
		if(checkForTermination(offset)==TERMINATE_EXP)
			return TERMINATE_EXP;
		
		clock_gettime(CLOCK_MONOTONIC, &t1);	

		addr_len = sizeof(sa);
		len = recvfrom(sock, (void*) measurement, packet_len, 0, (struct sockaddr *)&sa,
			&addr_len);
		
		if(len < 0 ){
			die("error first receive ", offset);
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		
		uint64_t sampleRtt = (timespec_diff_highres(&end, &start));
		estimatedRtt = 0.875 * estimatedRtt + 0.125 * sampleRtt;	
		
		
		time_elapsed = timespec_diff_lowres(&end, &start);

		uint32_t isInterval = time_elapsed % exp_options->interval_duration == 0;	
		if(isInterval && prevtime!=time_elapsed){
			double one_way_delay = estimatedRtt / 2;
			printf("One way delay is %f\n", one_way_delay);
			prevtime = time_elapsed;
		}
	}
}

/* Optional arg address for the server to bind 			*/
int start_experiment_server(int sock, const experiment_options_t *exp_options ){
	jitter_deviation_t jitters;	
	struct sockaddr_in sa;
	void *buff;
	char *addr = NULL;
	char file_to_write[50];
	struct udp_header *header_1;
	int len, addr_len, tmp, missing_packets = 0, prev_id = 0, curr_id = 0;
	int packet_len, haveMissedPacket, total_packets = 0;
	struct timespec start, end, t1, t2, t3;
	uint64_t prevtime=0U, time_elapsed;
	uint64_t secondssincelastprint=0U; //used to print in specified intervals
	uint64_t high_res_time = 0U, prev_high_res_time = 0U; 
	uint64_t jitter_sum = 0U, jitter_average = 0U, jitter_counter = 0U;
	uint32_t interval_min = 0U, interval_max = 0U;
	statistics_t stats;	
	short experstartflag=0;
	tcp_header_t termin_header;
	int ready;
	int offset = exp_options->offset;
	uint32_t missing_packets_so_far = 0, total_packets_so_far = 0; 
	int file_fd;
 	
	initJittersArray(&jitters);


	if(offset > 1){
		char filename[50];
		sprintf(filename, "(server_threads)-thread#%d.txt",((experiment_options_t *)exp_options)->offset);
		file_fd = open(filename,O_CREAT | O_WRONLY | O_TRUNC,0644);
	}

	if(exp_options->offset == 0 || exp_options->offset == 1){
	    sprintf(file_to_write, "output%d.dat",exp_options->offset);
   }

	memset(&t1, 0, sizeof(t1));
	memset(&t2, 0, sizeof(t2));
	memset(&t3, 0, sizeof(t3));	
	packet_len = exp_options->packet_length + sizeof(udp_header_t);
	addr = exp_options->address;
	
	sa.sin_family = AF_INET;	
	sa.sin_port = htons(exp_options->port + offset);
	if(!addr){
		sa.sin_addr.s_addr = INADDR_ANY; 
	}else{
		int ret = inet_pton(AF_INET, addr, &(sa.sin_addr));
		if(ret == 0 ){
			die("Address to bind from server is not valid ", offset);
		}
	}
	if(bind(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0 ){
		die("server bind", offset);
	}
	
	buff = malloc(packet_len);
	if(!buff){
		free(buff);
		die("Malloc error in server experiment ", offset);
	}
	
	addr_len = sizeof(sa);
	/* Discard first packet */
	len = recvfrom(sock, buff, packet_len, 0, (struct sockaddr *)&sa,
 		&addr_len);

	if(len < 0 ){
		die("error first receive ", offset);
	}
	
	memset(&stats, 0,  sizeof(stats));
	clock_gettime(CLOCK_MONOTONIC, &start);
	len = 0;
	for(;;){

		haveMissedPacket = 0;

		struct pollfd udp_poll_fd[1];
		int ret;
		memset(udp_poll_fd,0,sizeof(udp_poll_fd));
		udp_poll_fd->fd=udp_fd_list[offset];
		udp_poll_fd->events=POLLIN ;
		ret = poll(udp_poll_fd, 1, 0);

		if(udp_poll_fd->revents!=POLLIN){
			continue;
		}		

		len = recvfrom(sock, buff, packet_len, 0, (struct sockaddr *)&sa,
			&addr_len);
		clock_gettime(CLOCK_MONOTONIC, &end);
		
		updateJitterTimes(&t1, &t2, &t3);
		
		if(len == -1 ){
			die("error recvfrom  ", offset);
		}
		stats.throughput += len - sizeof(udp_header_t);
		total_packets++;
		time_elapsed = timespec_diff_lowres(&end, &start);
					
		uint64_t jitter = calculateJitter(&t1, &t2, &t3);
		updateJittersArray(&jitters, jitter);
		jitter_sum  += jitter;
		jitter_counter++;
	
		uint32_t isInterval = time_elapsed % exp_options->interval_duration == 0;	
		if(isInterval && prevtime!=time_elapsed){
			uint32_t prev_max = interval_max;
			interval_max += exp_options->interval_duration;
			interval_min = prev_max;
			stats.sec_min = interval_min;
			stats.sec_max = interval_max;
			stats.jitter = jitter_sum / jitter_counter;
			stats.pct_loss_percentage = calculateLostPackets(
								missing_packets_so_far, total_packets_so_far);	
			stats.jitter_standard_deviation = getJitterStandardDeviation(&jitters);
			clearJittersArray(&jitters); 
			if(offset == 0 || offset == 1 )
				output_to_file(file_to_write, &stats);
 
			send_stats(&stats, offset);
			printStatisticsPerSeconds(&stats,offset, file_fd);
			memset(&stats, 0, sizeof(stats));
			missing_packets_so_far = 0;
			total_packets_so_far = 0;
			jitter_sum = 0; jitter_counter = 0;
			prevtime=time_elapsed;
		}
			
		if(len < sizeof(udp_header_t )){
			missing_packets++;
			haveMissedPacket = 1;	
		}
	

		udp_header_t *header = (udp_header_t *)buff;	

		prev_id = curr_id;
		curr_id = ntohl(header->packet_id);
		if( (prev_id + 1) != curr_id ) {
			missing_packets++;
			missing_packets_so_far++;
			
			haveMissedPacket = 1;
		}
		if(!haveMissedPacket){
			stats.goodput += len - sizeof(udp_header_t ); 
		}
		total_packets_so_far++;
		
		if(checkForTermination(offset)==TERMINATE_EXP)
			return TERMINATE_EXP;
	}
	freeJittersArray(&jitters);
	free(buff);	
}

int conduct_experiment_master_server(const experiment_options_t *input_options){
	
	char buff[100];
	int offset;
	pthread_t *threadsarr;
	experiment_options_t* received_options = init_iperf_server(input_options);
	

	if(received_options->parallel_streams > 1){
		threadsarr = malloc(sizeof(pthread_t) * 
				(received_options->parallel_streams + 1));
		init_multiple_iperf_streams(received_options, threadsarr);
	}else{
		received_options->offset = 0;
		received_options->isThread = 0;
		conduct_experiment_server(received_options);
	}
	free(threadsarr);
	free(received_options);
}


int conduct_experiment_server(const experiment_options_t *input_options){

	experiment_options_t* received_options = NULL;
	int offset;
	if(input_options->isThread == 1){	
		experiment_options_t* received_options = init_iperf_server(input_options);
	}
	offset = input_options->offset;
	int sock = get_udp_socket(offset);
	udp_fd_list[offset] = sock;
	if(input_options->one_way_delay){
		measure_one_way_delay_server(sock, input_options);
	}else{
		start_experiment_server(sock, input_options);
	}
	terminate_iperf_server(input_options);
	if(received_options)
		free(received_options);	
}

/*
	creates a new json files if it doesn't exists and
	appends the statistics into json format 
*/
void output_to_file(const char *filename, const statistics_t *stats){
	FILE *fp;
	fp = fopen(filename, "a");
	fprintf(fp, "%u %u %u %u %f %lu %lu\n",
				stats->socket_id, stats->sec_min, stats->throughput, stats->goodput, 
				stats->pct_loss_percentage, stats->jitter, stats->jitter_standard_deviation);
	fclose(fp);
}
