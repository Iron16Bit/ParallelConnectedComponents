// https://cp-algorithms.com/data_structures/disjoint_set_union.html
// https://www.geeksforgeeks.org/dsa/number-of-connected-components-of-a-graph-using-disjoint-set-union/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "readData.c"
#include "utils.c"

int* initParent(struct COO matrix) {
    int length = matrix.numberOfRows;
    int* parent = malloc(sizeof(int) * length);

    // Initialize each parent as the nodeitself
    for (int i = 0; i < length; i++) {
        // struct coordinates edge = getNElement(matrix, i);
        parent[i] = i;
    }

    return parent;
}

int root(int vertex, int* parent) {
    // Find the topmost parent of vertex
    if (vertex == parent[vertex]) {
        return vertex;
    }

    parent[vertex] = root(parent[vertex], parent);
    return parent[vertex];
}

void connectedComponents(struct COO matrix, int* parent, int* rank) {
    // Connect nodes with shared edges
    for (int i = 0; i < matrix.numberOfValues; i++) {
        struct coordinates edge = getNElement(matrix, i);
        int vertexA = root(edge.row, parent);
        int vertexB = root(edge.col, parent);

        if (vertexA != vertexB) {
            if (rank[vertexA] < rank[vertexB]) {
                swap(&vertexA, &vertexB);
            }
            parent[vertexB] = vertexA;
            if (rank[vertexA] == rank[vertexB]) {
                rank[vertexA] += 1;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error Usage: ./a.out <path_to_file>\n");
        exit(1);
    }

    struct timeval startTime, endTime;
    struct timezone tz;

    gettimeofday(&startTime, &tz);

    struct COO matrix;
    initStruct(&matrix, argv[1]);

    // printMatrix(matrix);

    int* parent = initParent(matrix);
    int* rank = calloc(matrix.numberOfRows, sizeof(int));
    connectedComponents(matrix, parent, rank);

    printSolution(parent, matrix.numberOfRows);
    gettimeofday(&endTime, &tz);

    double elapsedTime = (endTime.tv_usec - startTime.tv_usec)*1e-6;
    printf("Execution time: %fs\n", elapsedTime);

    return 0;
}