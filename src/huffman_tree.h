// these are the main things related to the binary tree that we need to create
// for Huffman coding
//
// for a leaf node, its symbol is one of the unique bytes (out of the 256
// possible bytes) in the file we want to compress. and its weight is the number
// of times that that byte appears (frequency) in the file we want to compress
//
// for a branch node, its symbol is unused and its weight is the sum of its
// children's weights
//
// when the tree itself is written to the compressed file and read from the
// compressed file, the weights are not included since they are not needed

#include <stdbool.h>
#include <stdlib.h>

// the word "node" is used everywhere, but more specifically, this is a node
// of the huffman tree
struct node {
    unsigned char symbol;
    int weight;

    struct node *left_child;
    struct node *right_child;
};

struct node *create_huffman_tree(const int frequencies[256]);
struct node *create_node(unsigned char symbol, int weight);

bool is_leaf_node(const struct node *node);
int compare_nodes(const void *first, const void *second);

void free_node_recursive(struct node *node);
