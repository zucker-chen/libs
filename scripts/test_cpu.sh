#!/bin/sh 

run_times=$1

# Note:function only support bash, dash do not support
#function while_test()
#{
#    while [ 1 ];
#    do
#		echo "12345678980" | md5sum > /dev/null;
#		sleep 0.01
#    done
#}
#function run_cpu()
#{
#	#for ((i=0; i<run_times; i++));
#	while [ $run_times -ne 0 ];
#	do
#		while_test &
#		let "run_times--"
#	done
#}
#run_cpu

# main
if [ -z "$1" ]; then
	printf "Error: Missing argument, need like: $0 5\n"
	exit 1
fi

while [ $run_times -ne 0 ];
do
	# Note:function only support bash, dash do not support
	(while [ 1 ]; do (echo "12345678980" | md5sum > /dev/null; sleep 0.01) done) &
	# Note: let only support bash, expr instead it.
	run_times=$(expr $run_times - 1)
done

