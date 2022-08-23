#!/bin/bash

if [ "$1" == "" ]; then
    echo "Error: You must enter a filename."
    echo "For example: $0 test-me.txt"
    exit 1
fi

if [ -f "$1".thf.reversed ]; then
    echo "Error: Decoder output file $1.thf.reversed already exists."
    exit 1
fi

if ! ./encoder "$1" ; then
    echo "Error: Encoder failed."
    exit 1
fi

if ! ./decoder "$1".thf > "$1".thf.reversed ; then
    echo "Error: Decoder failed."
    echo "You can now delete the following files that were created:"
    echo "      compressed file: \"$1.thf\""
    echo "    decompressed file: \"$1.thf.reversed\""
    exit 1
fi

# using "<" prevents wc from printing the filename after the number of bytes
FILE_ORIGINAL_SIZE=$(wc --bytes < "$1")
FILE_COMPRESSED_SIZE=$(wc --bytes < "$1".thf)
# this is only integer division, but that's ok for our purposes
PERCENTAGE=$((100 * $FILE_COMPRESSED_SIZE / $FILE_ORIGINAL_SIZE))
echo "Results:"
echo "The compressed file is $PERCENTAGE% the size of the original file."

if ! diff "$1" "$1".thf.reversed ; then
    echo "Error: The original file and the decompressed file differ."
    exit 1
else
    echo "The original file and the decompressed file match."
    echo "You can now delete the following files that were created:"
    echo "      compressed file: \"$1.thf\""
    echo "    decompressed file: \"$1.thf.reversed\""
    exit 0
fi
