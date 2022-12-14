#define _DEFAULT_SOURCE // for endian.h
#include "bitbuffer.h"
#include "huffman_tree.h"
#include <endian.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

// create node by reading the bits that should represent the node. if it's a
// branch node, do the same with its child nodes. returns whether the reading
// was successful
//
// see comment on write_huffman_tree() in encoder.c for how the tree was written
int read_node_recursive(
    FILE *file,
    struct bit_buffer *buffer,
    struct node **node,
    int depth
) {
    // we have gone past the maximum possible codeword length / depth (see
    // comment of prefix_code_mapping struct in encoder.c). this means that the
    // compressed file is invalid
    if (depth >= 256) {
        return 1;
    }

    if (buffer->length < 1) {
        buffer_append_byte(buffer, fgetc(file));
    }

    // 0 is a branch node; 1 is a leaf node
    if (buffer->bits[0] == 0) {
        buffer_drop_left_bits(buffer, 1);
        *node = create_node(0, -1); // we don't need the weight, so -1
        int left_exit_status = read_node_recursive(
            file,
            buffer,
            &((*node)->left_child),
            depth + 1
        );
        if (left_exit_status) {
            return 1;
        }
        int right_exit_status = read_node_recursive(
            file,
            buffer,
            &((*node)->right_child),
            depth + 1
        );
        if (right_exit_status) {
            return 1;
        }
    } else {
        buffer_drop_left_bits(buffer, 1);
        // make sure we have enough bits in buffer to get the node's symbol
        if (buffer->length < 8) {
            buffer_append_byte(buffer, fgetc(file));
        }
        unsigned char symbol = convert_bits_to_byte(buffer->bits, 8);
        *node = create_node(symbol, -1); // we don't need the weight, so -1
        buffer_drop_left_bits(buffer, 8);
    }
    return 0;
}

// reconstruct the huffman tree that is written in the compressed file. returns
// whether the reading was successful
int read_huffman_tree(FILE *file, struct node **tree) {
    struct bit_buffer buffer;
    buffer.length = 0;

    if (read_node_recursive(file, &buffer, tree, 0)) {
        // it would have too much depth
        return 1;
    }
    if (is_leaf_node(*tree)) {
        // it is just 1 leaf node
        return 1;
    } else {
        return 0;
    }
}

// if the node is a leaf, get its symbol and drop the bits used to get here;
// else, use the current bit to determine which child node to follow. returns
// whether the decoding was successful
int decode_codeword_recursive(
    struct bit_buffer *buffer,
    int buffer_index,
    const struct node *node,
    unsigned char *symbol
) {
    if (is_leaf_node(node)) {
        // remove from the buffer the number of bits we read to resolve this
        // codeword, which we can determine from the buffer index
        buffer_drop_left_bits(buffer, buffer_index);
        *symbol = node->symbol;
        return 0;
    }

    // we are on a branch node, but are out of bits in the buffer, so we don't
    // know which direction to go. this means that the compressed file is
    // invalid
    if (buffer_index == buffer->length) {
        return 1;
    }

    struct node *child_to_follow;
    if (buffer->bits[buffer_index] == false) {
        child_to_follow = node->left_child;
    } else {
        child_to_follow = node->right_child;
    }
    return decode_codeword_recursive(
        buffer,
        buffer_index + 1,
        child_to_follow,
        symbol
    );
}

// decode 1 codeword's worth of bits from the buffer into its symbol by using
// the bits as a path in the huffman tree. return whether the decoding was
// successful
int decode_codeword(
    struct bit_buffer *buffer,
    const struct node *tree,
    unsigned char *symbol
) {
    return decode_codeword_recursive(buffer, 0, tree, symbol);
}

// decode the encoded data from the input file and write it to the output file.
// returns whether the decoding was successful
int decode_data_and_write(
    FILE *file_in,
    const struct node *huffman_tree,
    FILE *file_out,
    uint32_t number_of_bytes_to_decode
) {
    struct bit_buffer buffer;
    buffer.length = 0;

    uint32_t number_of_bytes_decoded = 0;
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

        // there is not enough encoded data to decode the specified number of
        // bytes. this means that the compressed file is invalid
        if (buffer.length == 0) {
            return 1;
        }

        unsigned char symbol;
        if (decode_codeword(&buffer, huffman_tree, &symbol)) {
            return 2;
        }
        number_of_bytes_decoded += 1;
        fputc(symbol, file_out);
    } while (number_of_bytes_decoded < number_of_bytes_to_decode);

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(
            stderr,
            "Error: You must specify the name of the file you want to"
            " decompress.\nFor example: %s slss.compressed\n",
            argv[0]
        );
        return 1;
    }
    FILE *file_in = fopen(argv[1], "r");
    if (!file_in) {
        fprintf(stderr, "Error: Could not open input file.\n");
        return 1;
    }

    uint32_t number_of_bytes_to_decode;
    if (fread(&number_of_bytes_to_decode, sizeof (uint32_t), 1, file_in) < 1) {
        fprintf(
            stderr,
            "Error: Unable to read the number of bytes to decode.\n"
            "The compressed file is invalid.\n"
        );
        fclose(file_in);
        return 1;
    }
    // convert from big endian to host endianness
    number_of_bytes_to_decode = be32toh(number_of_bytes_to_decode);

    struct node *reconstructed_huffman_tree = NULL;
    if (read_huffman_tree(file_in, &reconstructed_huffman_tree)) {
        fclose(file_in);
        free_node_recursive(reconstructed_huffman_tree);
        fprintf(
            stderr,
            "Error: Unable to read a proper binary Huffman tree.\n"
            "The compressed file is invalid.\n"
        );
        return 1;
    }

    // now the pointer in file_in is at the first byte of the encoded data
    int decoding_exit_status = decode_data_and_write(
        file_in,
        reconstructed_huffman_tree,
        stdout,
        number_of_bytes_to_decode
    );

    // free/close everything
    fclose(file_in);
    free_node_recursive(reconstructed_huffman_tree);

    if (decoding_exit_status == 1) {
        fprintf(
            stderr,
            "Error: There was not enough encoded data to decode the specified"
            " number of bytes.\nThe compressed file is invalid.\n"
        );
        return 1;
    } else if (decoding_exit_status == 2) {
        fprintf(
            stderr,
            "Error: There was not enough encoded data to decode the last"
            " codeword.\nThe compressed file is invalid.\n"
        );
        return 1;
    } else {
        return 0;
    }
}
