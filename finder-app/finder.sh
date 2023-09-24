#!/bin/sh
# 
# Author: jqiao
if [ $# -lt 2 ]
then
	echo "Insufficient number of arguments"
    exit 1
else
    filesdir=$1
    searchstr=$2
fi
        
if [ -d ${filesdir} ]
then  
    echo "Valid directory"
else
    echo "${filesdir} is not a directory"
    exit 1
fi

numFiles=$(find ${filesdir} -type f | wc -l)
numOccur=$(grep -r -o ${searchstr} ${filesdir} | wc -l)

echo "The number of files are ${numFiles} and the number of matching lines are ${numOccur} in valid directory"

exit 0