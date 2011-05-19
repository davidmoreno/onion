cd 01-internal
FAILED=""

for i in $( find -maxdepth 1 -type f -executable | sort -n ); do
	$i
	if [ "$?" != "0" ]; then
		FAILED="$FAILED $i"
	fi
done

if [ "$FAILED" ]; then
	echo -e "\033[31mFAILED TESTS: $FAILED\033[0m"
	exit 1
else
	echo "OK"
	exit 0
fi
