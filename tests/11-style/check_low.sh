#!/bin/bash

echo "Checks source code uses the onion_low_* functions and not directly others"

BADF="malloc alloc calloc realloc strdup free pthread_create pthread_destroy pthread_join pthread_cancel pthread_detach pthread_exit pthread_sigmask"
PATH=${1:-../../src}
FILES=$(/bin/find $PATH -name "*.c" -or -name "*.h" | /bin/grep -v low.[ch] )

ERROR=0
#echo $FILES
for f in $BADF; do
	HAS_F="$( /bin/grep '[[:space:]=]'$f'[[:space:]]*(' $FILES -l )"
	if [ "$HAS_F" ]; then
		echo "$f: $( echo $HAS_F )"
		ERROR=1
	fi
done

exit $ERROR

