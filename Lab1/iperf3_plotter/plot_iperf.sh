#!/bin/bash


if [ $# -ne 2 ]; then
	echo "***************************************"
	echo "Usage: $0 <iperf_json_file> UDP|TCP"
	echo "***************************************"
fi

preprocessor.sh $1 $2 .

if [ $? -ne 0 ]; then
	exit 1
fi

cd results
if [ "$2" = "TCP" ]; then
	gnuplot /usr/bin/plot_throughput.plt 2> /dev/null
	gnuplot /usr/bin/plot_cwnd.plt 2> /dev/null
	gnuplot /usr/bin/plot_retransmits.plt 2> /dev/null
elif [ "$2" = "UDP" ]; then
	gnuplot /usr/bin/plot_throughput.plt 2> /dev/null
	gnuplot /usr/bin/plot_jitter.plt 2> /dev/null
	gnuplot /usr/bin/plot_pkts_lst_prc.plt 2> /dev/null
fi
