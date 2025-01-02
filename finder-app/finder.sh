#!/bin/sh
#
# Requirements:
# - Write a shell script finder-app/finder.sh as described below:
#   - Accepts the following runtime arguments: the first argument is a path to a directory on the filesystem, referred to below as filesdir; the second argument is a text string which will be searched within these files, referred to below as searchstr
#   - Exits with return value 1 error and print statements if any of the parameters above were not specified
#   - Exits with return value 1 error and print statements if filesdir does not represent a directory on the filesystem
#   - Prints a message "The number of files are X and the number of matching lines are Y" where X is the number of files in the directory and all subdirectories and Y is the number of matching lines found in respective files, where a matching line refers to a line which contains searchstr (and may also contain additional content).
#
# Example invocation:
#   finder.sh /tmp/aesd/assignment1 linux
#
# References:
#  - https://unix.stackexchange.com/questions/6979/count-total-number-of-occurrences-using-grep
# 


# accept and rename input parameters (filesdir, searchstr), check number of params, exit if not ok
if [ $# -lt 2 ]
then
	echo "Not enough parameters supplied, exiting."
	exit 1
else
	filesdir=$1
	searchstr=$2
fi

# check that filesdir is a directory, exit if not
if ! [ -d "$filesdir" ]
then
	echo "Not a directory, exiting."
	exit 1
fi

# count occurrences using grep
numofmatches=$(grep -o -r "$searchstr" "$filesdir" | wc -l)

# count files using find
numoffiles=$(find "$filesdir" -type f | wc -l)

# reply and return
echo "The number of files are " $numoffiles "and the number of matching lines are" $numofmatches
exit 0

