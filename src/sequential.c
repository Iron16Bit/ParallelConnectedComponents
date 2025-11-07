#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include "readData.c"
#include "utils.c"

int* initParent(struct COO matrix) {
    int numberOfValues = matrix.numberOfValues;
    int* parent = malloc(sizeof(int)*numberOfValues);

    // Initialize each parent as the nodeitself
    for (int i=0; i<numberOfValues; i++) {
        struct coordinates edge = getNElement(matrix, i);
        parent[i] = edge.row;
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

void connectedComponents(int* parent, struct COO matrix) {
    // Connect nodes with shared edges
    for (int i=0; i<matrix.numberOfValues; i++) {
        struct coordinates edge = getNElement(matrix, i);
        int vertexA = root(edge.row, parent);
        int vertexB = root(edge.col, parent);

        if (vertexA != vertexB) {
            parent[vertexB] = vertexA;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error Usage: ./a.out <path_to_file>\n");
        exit(1);
    }

    struct COO matrix;
    initStruct(&matrix, argv[1]);

    printMatrix(matrix);

    int* parent = initParent(matrix);

    connectedComponents(parent, matrix);

    printSolution(parent, matrix.numberOfValues);

    return 0;
}