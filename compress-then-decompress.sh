#!/bin/bash

if [ ! -f "encoder" -o ! -f "decoder" ]; then
    echo "Error: \"encoder\" and/or \"decoder\" not found."
    echo "Make sure you run build.sh first to compile the binaries."
    exit 1
fi

if [ "$1" == "" ]; then
    echo "Error: You must enter a filename."
    echo "For example: $0 sample-files/slss"
    exit 1
fi

COMPRESSED_FILE_NAME="$1.compressed"
DECOMPRESSED_FILE_NAME="$1.compressed.decompressed"

if [ -f "$COMPRESSED_FILE_NAME" ]; then
    echo "Error: Encoder output file $COMPRESSED_FILE_NAME already exists."
    exit 1
fi
if [ -f "$DECOMPRESSED_FILE_NAME" ]; then
    echo "Error: Decoder output file $DECOMPRESSED_FILE_NAME already exists."
    exit 1
fi

if ! ./encoder "$1" > "$COMPRESSED_FILE_NAME" ; then
    echo "Error: Encoder failed."
    echo "You can now delete the following file that was created:"
    echo "      compressed file: \"$COMPRESSED_FILE_NAME\""
    exit 1
fi

if ! ./decoder "$COMPRESSED_FILE_NAME" > "$DECOMPRESSED_FILE_NAME" ; then
    echo "Error: Decoder failed."
    echo "You can now delete the following files that were created:"
    echo "      compressed file: \"$COMPRESSED_FILE_NAME\""
    echo "    decompressed file: \"$DECOMPRESSED_FILE_NAME\""
    exit 1
fi

# using "<" prevents wc from printing the filename after the number of bytes
ORIGINAL_FILE_SIZE=$(wc --bytes < "$1")
COMPRESSED_FILE_SIZE=$(wc --bytes < "$COMPRESSED_FILE_NAME")
# this is only integer division, but that's ok for our purposes
PERCENTAGE=$((100 * $COMPRESSED_FILE_SIZE / $ORIGINAL_FILE_SIZE))
echo "Results:"
echo "The compressed file is $PERCENTAGE% the size of the original file."

if ! diff "$1" "$DECOMPRESSED_FILE_NAME" ; then
    echo "Error: The original file and the decompressed file differ."
    exit 1
else
    echo "The original file and the decompressed file match."
    echo "You can now delete the following files that were created:"
    echo "      compressed file: \"$COMPRESSED_FILE_NAME\""
    echo "    decompressed file: \"$DECOMPRESSED_FILE_NAME\""
    exit 0
fi
