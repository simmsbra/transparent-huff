// see huffman_tree.h for an explanation of how the huffman tree is used

#include "huffman_tree.h"

// create a huffman tree on the heap based on the given byte frequencies
struct node *create_huffman_tree(const int frequencies[256]) {
    // for each byte with a non-zero frequency, make a leaf node for it and put
    // it into the array of nodes
    struct node *nodes[256];
    int number_of_nodes_in_array = 0;
    for (int i = 0; i < 256; i += 1) {
        if (frequencies[i] > 0) {
            nodes[number_of_nodes_in_array] = create_node(i, frequencies[i]);
            number_of_nodes_in_array += 1;
        }
    }

    // handle the edge case of 1 node by adding a 2nd arbitrary node, so that we
    // end up with a proper binary tree instead of just 1 node
    if (number_of_nodes_in_array == 1) {
        nodes[number_of_nodes_in_array] = create_node(
            nodes[0]->symbol + 128 % 256, // use the "farthest away" symbol
            0
        );
        number_of_nodes_in_array += 1;
    }

    // construct a tree by replacing the 2 lowest-weight nodes with 1 new branch
    // node whose children are those 2 nodes and whose weight is the sum of
    // those 2 nodes' weights, and repeating until we are left with only 1
    // (branch) node, which is the root of the tree
    while (number_of_nodes_in_array > 1) {
        // sort the nodes by descending weight
        qsort(nodes, number_of_nodes_in_array, sizeof (*nodes), &compare_nodes);

        int weight_sum = nodes[number_of_nodes_in_array - 1]->weight
                       + nodes[number_of_nodes_in_array - 2]->weight;
        struct node *branch_node = create_node(0, weight_sum);
        branch_node->left_child  = nodes[number_of_nodes_in_array - 2];
        branch_node->right_child = nodes[number_of_nodes_in_array - 1];

        nodes[number_of_nodes_in_array - 2] = branch_node;
        number_of_nodes_in_array -= 1;
    }

    return nodes[0];
}

// create a new node on the heap for the huffman tree
struct node *create_node(unsigned char symbol, int weight) {
    struct node *result = malloc(sizeof (struct node));
    result->symbol = symbol;
    result->weight = weight;
    result->left_child = NULL;
    result->right_child = NULL;

    return result;
}

bool is_leaf_node(const struct node *node) {
    // since we know that a tree for huffman coding is a full binary tree, we
    // know that each node has either 0 or 2 children. so we can just check for
    // the presence of 1 child
    return node->left_child == NULL;
}

// compare two nodes by their weights in a way that produces a descending order
// when passed to qsort()
int compare_nodes(const void *first, const void *second) {
    // each argument is not a pointer to node; it is a pointer to a pointer to a
    // node, because the node array is an array of pointers to node -- not an
    // array of nodes
    int freq_one = (*(struct node **)first)->weight;
    int freq_two = (*(struct node **)second)->weight;

    return freq_two - freq_one;
}

void free_node_recursive(struct node *node) {
    if (node->left_child != NULL) {
        free_node_recursive(node->left_child);
    }
    if (node->right_child != NULL) {
        free_node_recursive(node->right_child);
    }
    free(node);
}
