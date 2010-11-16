#!/bin/bash

#normal download


../../examples/userver/userver . &
echo %1

wget -O test http://localhost:8080/01-userver.sh
DF=$( diff test 01-userver.sh )
rm test
if [ "$DF" ]; then
	echo "Error: Download did not give the right file"
	exit 1
fi

# get out of known places
wget -O test http://localhost:8080/../../../../../../../etc/passwd
DF=$( diff test ../../../../../../../etc/passwd )
rm test
if [ ! "$DF" ]; then
	echo "Download gave the file! and its out of allowed scope!"
	exit 1
fi 

echo "Test passed OK."

kill %1
