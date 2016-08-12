#!/bin/sh

if [ ! "$1" ]; then
	echo "Compile Run Loop: $0 <program> [arguments...]"
	echo
	echo "Compiles (make) a program, executes it, and watch for source file changes. If any change, recompile it all, and loop again."
	echo 
	echo "Optionally you can have the environmental variables:"
	echo "  SOURCEDIR to point to where the source files are"
	echo "  EXTRASOURCES to add more files for watch modification, for example sources that later generate the source files, or data"
	echo "  REGEX to set the regex to match for source files is SOURCEDIR is set. Default is *.cpp *.hpp *.html *.js *.css *.png *.jpg"
	echo
	echo "Using compulerunloop you can easily modify an onion program and have the changes 'instantly' applied.. but only if you"
	echo "dont use memory to store status."

	exit 0
fi

REGEX=${REGEX:-"*.cpp *.hpp *.html *.js *.css *.png *.jpg"}
if [ "$SOURCEDIR" ]; then
	for re in $REGEX; do
		EXTRASOURCES="$EXTRASOURCES $( find $SOURCEDIR -name "$re" )"
	done
fi

SOURCEDIR=${SOURCEDIR:-.}
ORIGSOURCES="$EXTRASOURCES $( readelf -w $1   | grep -A5 '(DW_TAG_compile_unit)' | grep DW_AT_name | awk -F: '{ print $4 }' )"

SOURCES=""
NOTFOUND=""
for f in $ORIGSOURCES; do
	if [ -e "$f" ]; then
		S="$f"
	else
		S="$( find "$SOURCEDIR" -name $f )"
	fi
	if [ ! "$S" ]; then
		NOTFOUND="$NOTFOUND $f"
	else
		SOURCES="$SOURCES $S"
	fi
done

echo
echo "Source files $SOURCES. "
echo
echo "Watching them. If they change, kill, recompile, execute and wait again."
echo
if [ "$NOTFOUND" ]; then
		echo
		echo "Could not find source files $NOTFOUND."
		echo "Consider setting SOURCEDIR environmental var. For example:"
		echo "SOURCEDIR=../.. $*"
		echo
fi


PROG=$1
shift
PIDFILE=$( tempfile 2>/dev/null )
if [ ! "$PIDFILE" ]; then
	PIDFILE="/tmp/crl-$$"
fi



killapp(){
	jobs -p > $PIDFILE
	PIDS=$( cat $PIDFILE )
	echo "Killing $PIDS."

	if [ "$PIDS" ]; then
		echo kill $PIDS
		kill $PIDS 2>/dev/null
		sleep 1
		kill -9 $PIDS 2>/dev/null
	fi
	
	if [ ! "$1" ]; then
		exit
	fi
	wait $PIDS
}

trap killapp INT TERM

while true; do
	PIDS=""
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
		jobs -p > $PIDFILE
		PIDS=$( cat $PIDFILE )
		rm $PIDFILE
		echo -n "Pid is $PID. Watching..."
	fi
	inotifywait -e close_write $SOURCES
	killapp noexit
	echo "done."
	echo "Recompile"
done

