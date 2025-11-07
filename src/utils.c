#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// #include "readData.c"

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
        if (!contains(uniqueParents, numberOfUniqueParents, parent[i])) {
            uniqueParents[numberOfUniqueParents] = parent[i];
            numberOfUniqueParents += 1;
        }
    }

    printf("Number of connected components: %d\n", numberOfUniqueParents);
}

void swap(int* a, int* b){
    int tmp = *a;
    *a = *b;
    *b = tmp;
}