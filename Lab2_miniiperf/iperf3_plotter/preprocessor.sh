#!/bin/bash

#Verify number of parameters is 3
if [ $# -ne 3 ]; then
        echo "Usage: $0 <iperf_json_input> <TCP|UDP> <output_directory>"
        exit 1
fi

#Check if passed parameter is a file
if [ ! -f $1 ]; then
        echo "Error: $1 is not a file. Quitting..."
        exit 2
fi

#Check if passed paramter is a valid JSON
jq empty < $1 >> /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "Error: $1 is not a valid JSON file. Quitting..."
	exit 3
fi

if [ "$2" = "TCP" ]; then
        # Convert iperf's JSON output to CSV
        res=`cat $1 | jq -r '.intervals[].streams[] | [.socket, .start, .end, .seconds, .bytes, .bits_per_second, .retransmits, .snd_cwnd, .rtt, .rttvar, .pmtu, .omitted] | @csv'`
elif [ "$2" = "UDP" ]; then
        res=`cat $1 | jq -r '.intervals[].streams[] | [.socket, .start, .end, .seconds, .bytes, .bits_per_second, .jitter_ms, .lost_percent, .packets, .omitted] | @csv'`
else 
        echo "error"
        exit -1
fi
mkdir -p $3
# Insert new lines on each record
# Sort the file (by default, will be sorted by socket, or by client)
# Redirect the output to a file in the specified directory named iperf.csv
echo $res | sed 's/ /\n/g' | sort > $3/"iperf.csv"

# Count the number of iperf client flows
if [ "$2" = "TCP" ]; then
  num_flows=`jq '.end.sum_sent.seconds' $1 | cut -d. -f1` 
elif [ "$2" = "UDP" ]; then
  num_flows=`jq '.end.sum.seconds' $1 | cut -d. -f1` 
fi

# Create a directory 'results' to store all flows data
mkdir -p $3/results
rm -rf $3/results/*

split -l $num_flows --numeric=1 --additional-suffix=".dat" $3/iperf.csv flow_
mv flow_* $3/results 2> /dev/null
curr=`pwd`
cd $3/results
for FILE in `ls`; do mv $FILE `echo $FILE | sed -e 's:^\(flow_\)0*::'` 2> /dev/null; done
cd $curr


for file in $3/results/*.dat; do
        if [ "$2" = "TCP" ]; then
	        awk -F, '{print ($1,int($2),int($3),($5/1024)/1024,$6/1024,$7,$8,$9,$10,$11,$12)}' $file | sort -n -k 2 > tmp
        elif [ "$2" = "UDP" ]; then
	        awk -F, '{print ($1,int($2),int($3),($5/1024)/1024,$6/1024,$7,$8,$9,$10)}' $file | sort -n -k 2 > tmp
        fi
	mv tmp $file 2> /dev/null
done

