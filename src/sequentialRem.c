// https://cp-algorithms.com/data_structures/disjoint_set_union.html
// https://www.geeksforgeeks.org/dsa/number-of-connected-components-of-a-graph-using-disjoint-set-union/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "readData.c"
#include "utils.c"

int* initParent(struct Graph graph) {
    int length = graph.numberOfNodes;
    int* parent = malloc(sizeof(int) * length);

    // Initialize each parent as the node itself
    for (int i = 0; i < length; i++) {
        parent[i] = i;
    }

    return parent;
}



// Rem's algorithm
void findCommonAncestor(int x, int y, int* parent) {
    int rootX = x, rootY = y;
    int tmp;
    while (parent[rootX] != parent[rootY]) {
        if (parent[rootX] < parent[rootY]) {
            if (rootX == parent[rootX]) {
                parent[rootX] = parent[rootY];
                break;
            }
            tmp = parent[rootX];
            parent[rootX] = parent[rootY];
            rootX = tmp;
        } else {
            if (rootY == parent[rootY]) {
                parent[rootY] = parent[rootX];
                break;
            }
            tmp = parent[rootY];
            parent[rootY] = parent[rootX];
            rootY = tmp;
        }
    }
}

void connectedComponents(struct Graph graph, int* parent) {
    // Connect nodes using adjacency lists
    for (int u = 0; u < graph.numberOfNodes; u++) {
        for (int k = 0; k < graph.degree[u]; k++) {
            int v = graph.neighbors[u][k];
            if (u == v)
                continue;  // skip edges to self
            findCommonAncestor(u, v, parent);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error Usage: ./a.out <path_to_file>\n");
        exit(1);
    }

    struct timeval startTime, endTime;

    gettimeofday(&startTime, 0);

    struct Graph graph;
    initStruct(&graph, argv[1]);

    // printGraph(graph);

    int* parent = initParent(graph);
    connectedComponents(graph, parent);

    printSolution(parent, graph.numberOfNodes);
    gettimeofday(&endTime, 0);

    long executionSeconds = endTime.tv_sec - startTime.tv_sec;
    long executionMicroseconds = endTime.tv_usec - startTime.tv_usec;
    double elapsedTime = executionSeconds + executionMicroseconds * 1e-6;
    printf("Execution time: %.6fs\n", elapsedTime);

    return 0;
}