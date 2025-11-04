#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include "readData.c"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error Usage: ./a.out <path_to_file>\n");
        exit(1);
    }

    struct COO matrix;
    initStruct(&matrix, argv[1]);

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
    return 0;
}