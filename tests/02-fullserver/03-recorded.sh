#!/bin/bash

#normal download

killall hello
../../examples/hello/hello &
REQUESTS="$( ls segfault-*.txt )"

for i in $REQUESTS; do
	echo $i
	cat $i | nc localhost 8080
	sleep 1
	if [ ! "$( pgrep hello )" ]; then
		echo "Segfault at hello at $i"
		exit 1
	fi
done


kill %1
 