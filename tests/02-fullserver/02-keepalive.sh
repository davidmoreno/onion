#!/bin/bash

#normal download

PORT=${PORT:-8080}

../../examples/oterm/oterm --no-ssl -p $PORT &

echo "**************"
echo "If finishes, then its ok"
echo "**************"

echo "No keep alive, status"
ab -n 100 -c 10 http://localhost:$PORT/status
OK1=$?
echo "No keep alive, term/status"
ab -n 100 -c 10 http://localhost:$PORT/term/status
OK2=$?
echo "Keep alive, status"
ab -n 100 -c 10 -k http://localhost:$PORT/status
OK3=$?
echo "Keep alive, term/status"
ab -n 100 -c 10 -k http://localhost:$PORT/term/status
OK4=$?

echo "Fast dirty curl test"

OK4=0
N=1
while [ $N -lt 100 ]; do
	curl -k -I http://localhost:$PORT/term/status http://localhost:$PORT/ http://localhost:$PORT/term/status > /tmp/curl-$$
	if [ "$( grep '[\>\<\{\}]' /tmp/curl-$$ )" ]; then
		OK4=$N # error
		cat /tmp/curl-$$
		break
	fi
	rm /tmp/curl-$$
	N=$[ $N +1 ]
done

OK5="0"
SIZE="$( curl http://localhost:$PORT/jquery-1.4.3.min.js 2>/dev/null | wc -c )"
if [ "$SIZE"  != "77746" ]; then
	OK6="s$SIZE"
fi


if [ "$OK1" == "0" ] && [ "$OK2" == "0" ] && [ "$OK3" == "0" ] && [ "$OK4" == "0" ] && [ "$OK5" == "0" ]; then
	echo "Test passed OK."
else
	echo "Error passing tests. $OK1 $OK2 $OK3 $OK4 $OK5"
fi

kill %1
