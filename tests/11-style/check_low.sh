#!/bin/bash

echo "Checks source code uses the onion_low_* functions and not directly others"

BADF="malloc alloc calloc realloc strdup free pthread_create pthread_destroy pthread_join pthread_cancel pthread_detach pthread_exit pthread_sigmask"
PATH=${1:-../../src}
FILES=$(/bin/find $PATH -name "*.c" -or -name "*.h" )

# echo $FILES
for f in $BADF; do
	if [ "$( /bin/grep "[\s=^]$f\s*(" $FILES -l )" ]; then
		echo $f
		/bin/grep "[\s=^]$f\s*(" $FILES -ln
	fi
done
