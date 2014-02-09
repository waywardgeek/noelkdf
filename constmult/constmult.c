#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint32_t *values;
uint32_t *prevValues;
uint32_t *nodes;
bool *visited;
uint32_t numNodes = 1;

// Create a new node.
static inline void createNode(uint32_t value, uint32_t prevValue, uint32_t targetValue) {
    // Having a 0 small value indicates a free shift of a prior value
    //printf("Creating node %u\n", value);
    if(value >> 1 > targetValue) {
        fprintf(stderr, "value too large\n");
        exit(1);
    }
    if(value == 0) {
        fprintf(stderr, "value == 0\n");
        exit(1);
    }
    if(nodes[value] != 0) {
        fprintf(stderr, "Recomputing value %u\n", value);
        exit(1);
    }
    values[numNodes] = value;
    prevValues[numNodes] = prevValue;
    nodes[value] = numNodes++;
}

// Add all powers of 2 times the value to the node table.
static inline void addPowersOfTwo(uint32_t value, uint32_t targetValue) {
    while(true) {
        if(value << 1 < value || value > targetValue) {
            return; // Overflow
        }
        value <<= 1;
        if(nodes[value] == 0) {
            createNode(value, value >> 1, targetValue);
        }
    }
}

// Print how to compute the node.
static void printNodePath(uint32_t value) {
    uint32_t nodeIndex = nodes[value];
    visited[value] = true;
    uint32_t bigPrevValue = prevValues[nodeIndex];
    if(bigPrevValue != 0 && !visited[bigPrevValue]) {
        printNodePath(bigPrevValue);
    }
    uint32_t smallPrevValue = 0;
    if(bigPrevValue != 0 && bigPrevValue << 1 != value) {
        if(bigPrevValue < value) {
            smallPrevValue = value - bigPrevValue;
        } else {
            smallPrevValue = bigPrevValue - value;
        }
        if(smallPrevValue != 0 && !visited[smallPrevValue]) {
            printNodePath(smallPrevValue);
        }
    }
    if(bigPrevValue << 1 == value) {
        printf("%u = %u << 1\n", value, bigPrevValue);
    } else if(bigPrevValue != 0 && smallPrevValue != 0) {
        if(bigPrevValue > value) {
            printf("%u = %u - %u\n", value, bigPrevValue, smallPrevValue);
        } else {
            printf("%u = %u + %u\n", value, bigPrevValue, smallPrevValue);
        }
    }
}

// Return the larger of the values.
static inline uint32_t max(uint32_t value1, uint32_t value2) {
    return value1 >= value2? value1 : value2;
}

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("Usage: constmult value\n");
    }
    uint32_t targetValue = atoi(argv[1]);
    printf("Allocating memory\n");
    values = (uint32_t *)calloc(2*targetValue+1, sizeof(uint32_t));
    prevValues = (uint32_t *)calloc(2*targetValue+1, sizeof(uint32_t));
    nodes = (uint32_t *)calloc(2*targetValue+1, sizeof(uint32_t));
    visited = (bool *)calloc(2*targetValue+1, sizeof(bool));
    printf("Done allocating memory\n");
    createNode(1, 0, targetValue);
    addPowersOfTwo(1, targetValue); // We get powers of two for free
    printf("Done adding powers of two\n");
    uint32_t maxNode = numNodes;
    uint32_t i;
    for(i = 0; nodes[targetValue] == 0; i++) {
        if(i % 10000 == 0) {
            printf("Processed %u nodes of %u\n", i, numNodes);
            fflush(stdout);
        }
        if(i == maxNode) {
            printf("Next level %u - %u\n", maxNode, numNodes);
            maxNode = numNodes;
        }
        uint32_t value1 = values[i];
        uint32_t j;
        for(j = 0; j < i; j++) {
            uint32_t value2 = values[j];
            uint64_t value = (uint64_t)values[i] + values[j];
            if(value < 2*targetValue) {
                if(nodes[value] == 0) {
                    createNode(value, max(value1, value2), targetValue);
                    addPowersOfTwo(value, targetValue);
                }
            }
            if(value1 >= value2) {
                value = value1 - value2;
            } else {
                value = value2 - value1;
            }
            if(nodes[value] == 0) {
                createNode(value, max(value1, value2), targetValue);
                addPowersOfTwo(value, targetValue);
            }
        }
    }
    printNodePath(targetValue);
    return 0;
}
