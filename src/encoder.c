// compressed file format (see relevant functions for more details):
// 1. 32 bits for the number of bytes that were encoded using the prefix code
//      (we need this to know when the encoded data stops)
//      32 bit unsigned big-endian integer
// 2. the tree used to create the prefix code during huffman coding
// 3. 0-7 empty bits to align to byte boundary
// 4. the input bytes encoded with the prefix code
// 5. 0-7 empty bits to align to byte boundary

#define _DEFAULT_SOURCE // for endian.h
#include "bitbuffer.h"
#include "huffman_tree.h"
#include <ctype.h>
#include <endian.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct prefix_code_mapping {
    // in our case, each symbol will be a unique byte
    unsigned char symbol;
    // representing the codeword as a dynamic array of booleans (bits) is simple
    // but probably not memory efficient
    bool *codeword;
    // the maximum length of a huffman codeword (among a space of n symbols)
    // is n - 1 according to
    // https://inst.eecs.berkeley.edu/~cs170/fa18/assets/dis/dis05-sol.pdf
    //
    // so for our n of 256, the maximum length is 255
    unsigned char codeword_length;
};


// fill the "byte_frequencies" array with how many occurances each byte has in
// the file, and return the total number of bytes read
uint32_t count_byte_frequencies(
    FILE *file,
    int byte_frequencies[256]
) {
    for (int i = 0; i < 256; i += 1) {
        byte_frequencies[i] = 0;
    }
    uint32_t number_of_bytes_read = 0;

    int byte = fgetc(file);
    while (byte != EOF) {
        byte_frequencies[byte] += 1;
        number_of_bytes_read += 1;

        byte = fgetc(file);
    }

    return number_of_bytes_read;
}

// print the given byte as a decimal number and, if it's printable, the
// character it represents
void print_byte_as_number_and_character(unsigned char byte) {
    fprintf(stderr, "%3d", byte);

    if (isprint(byte)) {
        fprintf(stderr, " (%c)", byte);
    } else {
        fprintf(stderr, "    ");
    }
}

void print_byte_frequencies(const int byte_frequencies[256]) {
    fprintf(stderr, "Byte Frequencies:\n");
    for (int i = 0; i < 256; i += 1) {
        if (byte_frequencies[i] > 0) {
            print_byte_as_number_and_character(i);
            fprintf(stderr, ": %d\n", byte_frequencies[i]);
        }
    }
    fprintf(stderr, "\n");
}

// print the tree structure characters and indents from the path taken to get to
// this node, the node's weight, and the node's symbol (if it's a leaf node).
// then do the same for all nodes below it (if it's a branch node)
void print_node_recursive(
    bool path[255],
    int path_length,
    const struct node *node
) {
    for (int i = 0; i < path_length; i += 1) {
        if (i == path_length - 1) {
            if (path[i]) {
                fprintf(stderr, "└ ");
            } else {
                fprintf(stderr, "├ ");
            }
        } else {
            if (path[i]) {
                fprintf(stderr, "  ");
            } else {
                fprintf(stderr, "│ ");
            }
        }
    }

    fprintf(stderr, "%d", node->weight);

    if (is_leaf_node(node)) {
        fprintf(stderr, ": ");
        print_byte_as_number_and_character(node->symbol);
        fprintf(stderr, "\n");
    } else {
        fprintf(stderr, "\n");
        path[path_length] = false;
        print_node_recursive(path, path_length + 1, node->left_child);
        path[path_length] = true;
        print_node_recursive(path, path_length + 1, node->right_child);
    }
}

void print_huffman_tree(const struct node *root) {
    fprintf(stderr, "Huffman Tree:\n");
    bool path[255] = {false}; // initialize with all falses
    int path_length = 0;
    print_node_recursive(path, path_length, root);
    fprintf(stderr, "\n");
}

void print_prefix_code_mappings(
    const struct prefix_code_mapping mappings[256]
) {
    fprintf(stderr, "Prefix Code (Symbol-to-Codeword Mappings):\n");
    for (int i = 0; i < 256; i += 1) {
        if (mappings[i].codeword_length != 0) {
            print_byte_as_number_and_character(i);
            fprintf(stderr, ": ");
            for (int j = 0; j < mappings[i].codeword_length; j += 1) {
                fprintf(stderr, "%d", mappings[i].codeword[j]);
            }
            fprintf(stderr, "\n");
        }
    }
    fprintf(stderr, "\n");
}

// if the node is a leaf, create its prefix code mapping from the node's symbol
// and the path taken to get to the node; else, attempt that for all nodes below
//
// the codeword of each prefix code mapping is on the heap
void create_mapping_from_node_recursive(
    bool *path,
    int path_length,
    const struct node *node,
    struct prefix_code_mapping mappings[256]
) {
    if (is_leaf_node(node)) {
        mappings[node->symbol].codeword = malloc(path_length * sizeof (bool));
        for (int i = 0; i < path_length; i += 1) {
            mappings[node->symbol].codeword[i] = path[i];
        }
        mappings[node->symbol].codeword_length = path_length;
    } else {
        path[path_length] = false;
        create_mapping_from_node_recursive(
            path,
            path_length + 1,
            node->left_child,
            mappings
        );
        path[path_length] = true;
        create_mapping_from_node_recursive(
            path,
            path_length + 1,
            node->right_child,
            mappings
        );
    }
}

// create the prefix code mappings from the huffman tree. each prefix code
// mapping has some data on the heap
void create_prefix_code_mappings(
    const struct node *huffman_tree_root,
    struct prefix_code_mapping mappings[256]
) {
    for (int i = 0; i < 256; i += 1) {
        mappings[i].symbol = i;
        mappings[i].codeword = NULL;
        mappings[i].codeword_length = 0;
    }

    bool path[255] = {false}; // initialize with all falses
    create_mapping_from_node_recursive(path, 0, huffman_tree_root, mappings);
}

void free_prefix_code_mappings(struct prefix_code_mapping mappings[256]) {
    for (int i = 0; i < 256; i += 1) {
        free(mappings[i].codeword);
    }
}

// writes the huffman tree to the file, in a depth-first pre-order traversal
//
// branch nodes are represented as a 0 bit
// leaf nodes are represented as a 1 bit followed by their symbol/char
void write_huffman_tree(
    FILE *file,
    struct bit_buffer *buffer,
    const struct node *root
) {
    if (is_leaf_node(root)) {
        buffer_append_bit(buffer, true);
        buffer_append_byte(buffer, root->symbol);
        buffer_write_any_complete_bytes(file, buffer);
    } else {
        buffer_append_bit(buffer, false);
        buffer_write_any_complete_bytes(file, buffer);

        write_huffman_tree(file, buffer, root->left_child);
        write_huffman_tree(file, buffer, root->right_child);
    }
}

// for each byte of the input file, write that byte's codeword (according to the
// prefix code) to the output file
void write_encoded_data(
    FILE *file_in,
    const struct prefix_code_mapping mappings[256],
    struct bit_buffer *buffer,
    FILE *file_out
) {
    fseek(file_in, 0, SEEK_SET);

    while (true) {
        int symbol = fgetc(file_in);
        if (symbol == EOF) {
            return;
        }

        buffer_append_bits(
            buffer,
            mappings[symbol].codeword,
            mappings[symbol].codeword_length
        );
        buffer_write_any_complete_bytes(file_out, buffer);
    }
}

// see the top of this file for an outline of the compressed file format
void write_compressed_file(
    FILE *file_in,
    FILE *file_out,
    uint32_t number_of_bytes_to_encode,
    const struct node *huffman_tree_root,
    const struct prefix_code_mapping *mappings
) {
    struct bit_buffer buffer;
    buffer.length = 0;

    // convert from host endianness to big endian
    number_of_bytes_to_encode = htobe32(number_of_bytes_to_encode);
    fwrite(&number_of_bytes_to_encode, sizeof (uint32_t), 1, file_out);

    write_huffman_tree(file_out, &buffer, huffman_tree_root);
    buffer_write_any_leftover_bits_as_byte(file_out, &buffer);

    write_encoded_data(file_in, mappings, &buffer, file_out);
    buffer_write_any_leftover_bits_as_byte(file_out, &buffer);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(
            stderr,
            "Error: You must specify the name of the file you want to compress."
            "\nFor example: %s sample-files/slss\n",
            argv[0]
        );
        return 1;
    }
    FILE *file_in = fopen(argv[1], "r");
    if (!file_in) {
        fprintf(stderr, "Error: Could not open input file.\n");
        return 1;
    }

    int byte_frequencies[256];
    uint32_t total_bytes = count_byte_frequencies(
        file_in,
        byte_frequencies
    );
    // the goal of this project is not to make the most robust compressor, so
    // instead of handling edge cases that don't produce a proper binary tree
    // and that would require special logic, we'll just not support it
    if (total_bytes < 2) {
        fprintf(
            stderr,
            "Error: Compressing a file under 2 bytes is not supported.\n"
        );
        fclose(file_in);
        return 1;
    }

    // create a huffman tree using the bytes as the symbols and their
    // frequencies as the weights
    struct node *huffman_tree = create_huffman_tree(byte_frequencies);

    // create the prefix code mappings from the huffman tree. this will be our
    // dictionary for the actual encoding
    struct prefix_code_mapping mappings[256];
    create_prefix_code_mappings(huffman_tree, mappings);

    write_compressed_file(file_in, stdout, total_bytes, huffman_tree, mappings);

    print_byte_frequencies(byte_frequencies);
    print_huffman_tree(huffman_tree);
    print_prefix_code_mappings(mappings);

    // free/close everything
    fclose(file_in);
    free_node_recursive(huffman_tree);
    free_prefix_code_mappings(mappings);

    return 0;
}
