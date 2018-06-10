#!/bin/sh

INDENT=indent
OPTIONS="-linux -i2 -nut -brf"
FILES=$( find src tools tests examples -name "*.c" -or -name "*.h" )

echo "Indenting files..."
$INDENT $OPTIONS $FILES
echo "Done"
