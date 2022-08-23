#!/bin/bash

# compile with debugging symbols, a lot of warnings, and the C17 standard
gcc -g -Wall -Wextra -std=c17 \
    src/bitbuffer.c src/huffman_tree.c src/encoder.c \
    -o encoder

gcc -g -Wall -Wextra -std=c17 \
    src/bitbuffer.c src/huffman_tree.c src/decoder.c \
    -o decoder
