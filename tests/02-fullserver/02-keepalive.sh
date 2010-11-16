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

if [ "$OK1" == "0" ] && [ "$OK2" == "0" ] && [ "$OK3" == "0" ] && [ "$OK4" == "0" ]; then
	echo "Test passed OK."
else
	echo "Error passing tests. $OK1 $OK2 $OK3 $OK4 "
fi

kill %1
