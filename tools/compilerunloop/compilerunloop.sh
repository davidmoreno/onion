#!/bin/sh

if [ ! "$1" ]; then
	echo "Compile Run Loop: $0 <program> [arguments...]"
	echo
	echo "Compiles (make) a program, executes it, and watch for source file changes. If any change, recompile it all, and loop again."
	echo 
	echo "Optionally you can have the environmental variable SOURCEDIR to point to where the source files are"
	echo
	echo "Using compulerunloop you can easily modify an onion program and have the changes 'instantly' applied.. but only if you"
	echo "dont use memory to store status."

	exit 0
fi

if [ ! "$SOURCEDIR" ]; then
	SOURCEDIR=.
fi

SOURCES=""
for f in $( objdump -x $1 | grep '\sdf\s' | grep 'c$' | awk '{ print $6 }' | sort | uniq ); do
	S=$( find "$SOURCEDIR" -name $f )
	if [ ! "$S" ]; then
		echo
		echo "Could not find source file $f. Consider setting SOURCEDIR environmental var. For example:"
		echo "SOURCEDIR=../.. $*"
		echo
	else
		SOURCES="$SOURCES $S"
	fi
done

echo
echo "Source files $SOURCES. Watching them. If they change, kill, recompile, execute and wait again."
echo

PROG=$1
shift
PIDFILE=$( tempfile )

while true; do
	make
	if [ "$?" != "0" ]; then
		echo "Error compiling. Just waiting"
	else
		if [ "$*" ]; then
			$PROG $* &
		else
			$PROG &
		fi
		sleep 2
		jobs
		jobs -p > $PIDFILE
		PIDS=$( cat $PIDFILE )
		rm $PIDFILE
		echo -n "Pid is $PID. Watching..."
	fi
	inotifywait -e close_write $SOURCES
	kill $PIDS
	sleep 1
	kill -9 $PIDS 2>/dev/null
	echo "done."
	echo "Recompile"
done

