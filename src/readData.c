#include <stdio.h>
#include <stdlib.h>

struct Graph {
    int* neighbors;    // Neighbors[u]: an array of neighbor ids
    int* degree;        // Degree[u]: number of neighbors for node u
    int numberOfNodes;  // Number of nodes
    int numberOfEdges;  // Number of edge entries (lines) in the file
};

int initStruct(struct Graph* graph /* out */,
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

    // Ignore comment on first line
    getline(&line, &len, fp);

    getline(&line, &len, fp);
    if (sscanf(line, "%d %d %d", &rows, &cols, &values) != 3) {
        fprintf(stderr, "Error: Failed to parse matrix dimensions\n");
        fclose(fp);
        return 1;
    }

    int* degree = (int*)calloc(rows, sizeof(int));
    int* neighbors = (int*)malloc(sizeof(int) * values);

    // Buffer for the current row
    int current_row = -1;
    int indexNeighbors = 0;

    while (getline(&line, &len, fp) != -1) {
        int r = 0, c = 0;
        if (sscanf(line, "%d %d", &r, &c) < 2) {
            fprintf(stderr, "Error: Failed to parse matrix dimensions\n");
            fclose(fp);
            return 1;
        }

        if (r != current_row && current_row != -1) {
            for (int i = current_row; i < r; i++) {
                degree[i] = 0;
                current_row = r;
            }
        }
        neighbors[indexNeighbors] = c;
        indexNeighbors += 1;
        degree[r] += 1;
    }

    graph->neighbors = neighbors;
    graph->degree = degree;
    graph->numberOfNodes = rows;
    graph->numberOfEdges = indexNeighbors;

    fclose(fp);
    if (line)
        free(line);

    return 0;
}