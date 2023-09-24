#!/bin/sh
# 
# Author: jqiao
if [ $# -lt 2 ]
then
	echo "Insufficient number of arguments"
    exit 1
else
    writefile=$1
    writestr=$2
fi
        
mkdir -p "$(dirname $writefile)" && touch "$writefile"

if test -f "$writefile"; then
    echo "$writefile created."
    else
    echo "cannot create file"
    exit 1
fi


echo ${writestr} >> ${writefile}

exit 0