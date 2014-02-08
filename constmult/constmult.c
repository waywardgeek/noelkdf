#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct NodeStruct *Node;

struct NodeStruct {
    Node bigPrevNode;
    Node smallPrevNode;
    uint32_t value;
    uint32_t numOperations; // This is the max of numOperations of prev nodes + 1
    bool subtract;         // True if we subtractted the smaller value, rather than add
    bool visited;
};

// Create a new node.
static Node createNode(Node *nodes, uint32_t value, Node bigPrevNode, Node smallPrevNode, bool subtract) {
    // Having a 0 small value indicates a free shift of a prior value
    Node node = (struct NodeStruct *)calloc(1, sizeof(struct NodeStruct));
    node->value = value;
    node->bigPrevNode = bigPrevNode;
    node->smallPrevNode = smallPrevNode;
    uint32_t numOperations = 0;
    if(bigPrevNode != 0) {
        numOperations = bigPrevNode->numOperations;
        if(smallPrevNode != 0) {
            if(smallPrevNode->numOperations > numOperations) {
                numOperations = smallPrevNode->numOperations;
            }
            numOperations++;
        }
    }
    node->numOperations = numOperations;
    node->subtract = subtract;
    nodes[value] = node;
    return node;
}

// Add all powers of 2 times the value to the node table.
static void addPowersOfTwo(Node *nodes, Node prevNode, uint32_t targetValue) {
    uint32_t value = prevNode->value;
    while(value < targetValue*2) {
        if(nodes[value] == NULL) {
            prevNode = createNode(nodes, value, prevNode, NULL, false);
        }
        value <<= 1;
    }
}

// Given all the values in the node table, add all the values we could compute with one
// more adder or subtractor.  Basically, for each node that can be computed with exactly
// numOperations, find all the smaller nodes computed with numOperations or less, and add
// or subtract it.  Then, add all powers of two to the table.
static void addOneAdderOrSubtractor(Node *nodes, uint32_t targetValue, uint32_t numOperations) {
    uint32_t i;
    for(i = 1; i < targetValue*2; i++) {
        Node bigNode = nodes[i];
        if(bigNode != NULL && bigNode->numOperations == numOperations) {
            uint32_t j;
            for(j = 1; j < i; j++) {
                Node smallNode = nodes[j];
                Node node;
                if(smallNode != NULL && smallNode->numOperations <= numOperations) {
                    if(i + j < 2*targetValue) {
                        node = nodes[i + j];
                        if(node == NULL) {
                            node = createNode(nodes, i + j, bigNode, smallNode, false);
                            addPowersOfTwo(nodes, node, targetValue);
                        }
                    }
                    node = nodes[i - j];
                    if(node == NULL) {
                        node = createNode(nodes, i - j, bigNode, smallNode, false);
                        addPowersOfTwo(nodes, node, targetValue);
                    }
                }
            }
        }
    }
}

// Print how to compute the node.
static void printNodePathRec(Node node) {
    node->visited = true;
    Node smallPrevNode = node->smallPrevNode;
    Node bigPrevNode = node->bigPrevNode;
    if(smallPrevNode != NULL && !smallPrevNode->visited) {
        printNodePathRec(smallPrevNode);
    }
    if(bigPrevNode != NULL && !bigPrevNode->visited) {
        printNodePathRec(bigPrevNode);
    }
    if(bigPrevNode != NULL && smallPrevNode != NULL) {
        if(node->subtract) {
            printf("%u = %u - %u\n", node->value, bigPrevNode->value, smallPrevNode->value);
        } else {
            printf("%u = %u + %u\n", node->value, bigPrevNode->value, smallPrevNode->value);
        }
    } else if(bigPrevNode != NULL) {
        printf("%u = %u << 1\n", node->value, bigPrevNode->value);
    }
}

// Print how to compute the node.
static void printNodePath(Node node) {
    uint32_t i;
    for(i = 0; i < node->value; i++) {
        node->visited = false;
    }
    printf("%u can be computed in %u operations\n", node->value, node->numOperations);
    printNodePathRec(node);
}

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("Usage: constmult value\n");
    }
    uint32_t targetValue = atoi(argv[1]);
    Node *nodes = (Node *)calloc(2*targetValue+1, sizeof(Node));
    Node node = createNode(nodes, 1, NULL, NULL, false);
    nodes[1] = node;
    addPowersOfTwo(nodes, node, targetValue); // We get powers of two for free
    uint32_t i;
    for(i = 0; nodes[targetValue] == NULL; i++) {
        addOneAdderOrSubtractor(nodes, targetValue, i);
    }
    printNodePath(nodes[targetValue]);
}
