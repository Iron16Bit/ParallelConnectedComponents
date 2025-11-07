#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int root(int vertex, int* parent);

struct coordinates {
    int row;
    int col;
};

void printMatrix(struct COO matrix) {
    printf("Cols:\n");
    for (int i = 0; i < matrix.numberOfValues; i++) {
        printf("%d, ", matrix.cols[i]);
    }
    printf("\n");

    printf("RowPointer:\n");
    for (int i = 0; i < matrix.numberOfRows; i++) {
        printf("%d, ", matrix.rowPointer[i]);
    }
    printf("\n");
}

struct coordinates getNElement(struct COO matrix, int n) {
    struct coordinates retVal;

    // TODO implement binary search
    for (int i = 0; i < matrix.numberOfRows; i++) {
        if (matrix.rowPointer[i] > n) {
            retVal.row = i;
            break;
        }
    }

    retVal.col = matrix.cols[n];
    return retVal;
}

bool contains(int* parent, int length, int n) {
    for (int i = 0; i < length; i++) {
        if (parent[i] == n) {
            return true;
        }
    }
    return false;
}

void printSolution(int* parent, int lenght) {
    int* uniqueParents = (int*)malloc(sizeof(int) * lenght);
    int numberOfUniqueParents = 0;

    for (int i = 0; i < lenght; i++) {
        int vertexRoot = root(i, parent);
        if (!contains(uniqueParents, numberOfUniqueParents, vertexRoot)) {
            uniqueParents[numberOfUniqueParents] = vertexRoot;
            numberOfUniqueParents += 1;
        }
    }

    printf("Number of connected components: %d\n", numberOfUniqueParents);
}

void swap(int* a, int* b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}