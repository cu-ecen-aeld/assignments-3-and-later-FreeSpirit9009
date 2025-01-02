#
# requirements:
# - Accepts the following arguments:
#   - the first argument is a full path to a file (including filename) on the filesystem, referred to below as writefile;
#   - the second argument is a text string which will be written within this file, referred to below as writestr
# - Exits with value 1 error and print statements if any of the arguments above were not specified
# - Creates a new file with name and path writefile with content writestr, overwriting any existing file and creating the path if it doesn’t exist.
# - Exits with value 1 and error print statement if the file could not be created.

# accept and rename input parameters (writefile, writestr), check number of params, exit if not ok
if [ $# -lt 2 ]
then
	echo "Not enough parameters supplied, exiting."
	exit 1
else
	writefile=$1
	writestr=$2
fi

# Creates a new file with name and path writefile with content writestr, overwriting any existing file and creating the path if it doesn’t exist. 
# If path does not exist, create it
if ! [ -e "$(dirname "$writefile")" ]
then
	# -p needed so that multiple level of directories can be created
	mkdir -p $(dirname "$writefile")
	if [ $? -ne 0 ]
	then
		echo "could not create directory"
		exit 1
	fi
fi

# Write to file
echo $writestr > $writefile

# Exits with value 1 and error print statement if the file could not be created.
if [ $? -ne 0 ]
then
	echo "file not create directory or written to"
	exit 1
fi
exit 0

