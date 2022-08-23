#include "bitbuffer.h"
#include "huffman_tree.h"
#include <stdbool.h>
#include <stdio.h>

// create node by reading the bits that should represent the node. if it's a
// branch node, do the same with its child nodes.
//
// see comment on write_huffman_tree() in encoder.c for how the tree was written
struct node *read_node_recursive(FILE *file, struct bit_buffer *buffer) {
    if (buffer->length < 1) {
        buffer_append_byte(buffer, fgetc(file));
    }

    struct node *node;
    // 0 is a branch node; 1 is a leaf node
    if (buffer->bits[0] == 0) {
        buffer_drop_left_bits(buffer, 1);
        node = create_node(0, -1); // we don't need the weight, so -1
        node->left_child = read_node_recursive(file, buffer);
        node->right_child = read_node_recursive(file, buffer);
    } else {
        buffer_drop_left_bits(buffer, 1);
        // make sure we have enough bits in buffer to get the node's symbol
        if (buffer->length < 8) {
            buffer_append_byte(buffer, fgetc(file));
        }
        unsigned char symbol = convert_bits_to_byte(buffer->bits, 8);
        node = create_node(symbol, -1); // we don't need the weight, so -1
        buffer_drop_left_bits(buffer, 8);
    }

    return node;
}

// reconstruct the huffman tree that is written in the compressed file
struct node *read_huffman_tree(FILE *file) {
    struct bit_buffer buffer;
    buffer.length = 0;

    return read_node_recursive(file, &buffer);
}

// if the node is a leaf, return its symbol and drop the bits used to get here;
// else, use the current bit to determine which child node to follow
unsigned char decode_codeword_recursive(
    struct bit_buffer *buffer,
    int buffer_index,
    const struct node *node
) {
    if (is_leaf_node(node)) {
        // remove from the buffer the number of bits we read to resolve this
        // codeword, which we can determine from the buffer index
        buffer_drop_left_bits(buffer, buffer_index);
        return node->symbol;
    }

    struct node *child_to_follow;
    if (buffer->bits[buffer_index] == false) {
        child_to_follow = node->left_child;
    } else {
        child_to_follow = node->right_child;
    }
    return decode_codeword_recursive(buffer, buffer_index + 1, child_to_follow);
}

// decode 1 codeword's worth of bits from the buffer into its symbol by using
// the bits as a path in the huffman tree
unsigned char decode_codeword(
    struct bit_buffer *buffer,
    const struct node *tree
) {
    return decode_codeword_recursive(buffer, 0, tree);
}

// decode the encoded data from the input file and write it to the output file
void decode_data_and_write(
    FILE *file_in,
    const struct node *huffman_tree,
    FILE *file_out,
    unsigned long int number_of_bytes_to_decode
) {
    struct bit_buffer buffer;
    buffer.length = 0;

    unsigned long int number_of_bytes_decoded = 0;
    bool have_reached_end_of_file = false;
    do {
        // the longest codeword length possible is 255 (see prefix_code_mapping
        // struct comment in encoder.c), so we want to get at least that many
        // bits (unless we're at the end of the file) in order to make sure that
        // the codeword will reach a leaf node when traversing the tree
        while (buffer.length < 255 && !have_reached_end_of_file) {
            int value = fgetc(file_in);
            if (value != EOF) {
                buffer_append_byte(&buffer, (unsigned char)value);
            } else {
                have_reached_end_of_file = true;
            }
        }

        unsigned char symbol = decode_codeword(&buffer, huffman_tree);
        number_of_bytes_decoded += 1;
        fputc(symbol, file_out);
    } while (number_of_bytes_decoded < number_of_bytes_to_decode);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf(
            "Error: You must specify the name of the file you want to"
            " decompress.\nFor example: %s decompress-me.txt.thf\n",
            argv[0]
        );
        return 1;
    }
    FILE *file_in = fopen(argv[1], "r");
    if (!file_in) {
        puts("Error: Could not open input file.");
        return 1;
    }

    unsigned long int number_of_bytes_to_decode;
    fread(&number_of_bytes_to_decode, sizeof (unsigned long int), 1, file_in);
    struct node *reconstructed_huffman_tree = read_huffman_tree(file_in);
    // now the pointer in file_in is at the first byte of the encoded data
    decode_data_and_write(
        file_in,
        reconstructed_huffman_tree,
        stdout,
        number_of_bytes_to_decode
    );

    // free/close everything
    fclose(file_in);
    free_node_recursive(reconstructed_huffman_tree);

    return 0;
}
