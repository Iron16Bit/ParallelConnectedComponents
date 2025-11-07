#include <stdio.h>
#include <stdlib.h>

struct COO {
    int* rowPointer;
    int* cols;
    int numberOfRows;
    int numberOfValues;
    int numberOfCols;
};

int initStruct(struct COO* matrix /* out */,
               char* pathToFile /* in */) {
    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    int rows, cols, values;

    fp = fopen(pathToFile, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error File: %s is not found\n", pathToFile);
        exit(1);
    }

    // ignore first line
    getline(&line, &len, fp);

    // Reading first line to get matrix shape
    getline(&line, &len, fp);
    if (sscanf(line, "%d %d %d", &rows, &cols, &values) != 3) {
        fprintf(stderr, "Error: Failed to parse matrix dimensions\n");
        fclose(fp);
        return 1;
    }

    // Data Init
    // struct COO matrix;
    int* rowPointerArray = (int*)calloc(rows, sizeof(int));
    int* colsArray = (int*)calloc(values, sizeof(int));
    (*matrix).cols = (int*)colsArray;
    (*matrix).rowPointer = (int*)rowPointerArray;
    (*matrix).numberOfRows = rows;
    (*matrix).numberOfCols = cols;
    (*matrix).numberOfValues = values;

    int lastRow = 0;
    int row = 0;
    for (int i = 0; i < values; i++) {
        getline(&line, &len, fp);
        sscanf(line, "%d %d", &row, &cols);

        // rowPointer
        if (lastRow == row) {
            (*matrix).rowPointer[row] += 1;
        } else {
            for (int j = lastRow + 1; j <= row; j++) {
                (*matrix).rowPointer[j] = (*matrix).rowPointer[lastRow];
            }
            (*matrix).rowPointer[row] += 1;
        }

        // cols
        (*matrix).cols[i] = cols;

        // next
        lastRow = row;
    }

    fclose(fp);
    if (line)
        free(line);

    return 0;
}