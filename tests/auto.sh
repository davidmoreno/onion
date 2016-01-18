#!/bin/sh

FAILED=""

for d in 01-internal 02-fullserver; do 
	pushd $d
	for i in $( find -maxdepth 1 -type f -executable | sort -n ); do
		$i
		if [ "$?" != "0" ]; then
			FAILED="$FAILED $d/$i"
		fi
	done
	popd
done
	
if [ "$FAILED" ]; then
	echo -e "\033[31mFAILED TESTS: $FAILED\033[0m"
	exit 1
else
	echo "OK"
	exit 0
fi
