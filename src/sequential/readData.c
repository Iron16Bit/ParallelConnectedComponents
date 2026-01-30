#include <stdio.h>
#include <stdlib.h>

struct Graph {
    int** neighbors;    // Neighbors[u]: an array of neighbor ids
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
    int** neighbors = (int**)malloc(sizeof(int*) * rows);
    for (int i = 0; i < rows; i++) {
        neighbors[i] = NULL;
    }

    // Buffer for the current row
    int current_row = -1;
    int* buf = NULL;
    int buf_cap = 0;
    int buf_len = 0;
    long totalEntries = 0;

    while (getline(&line, &len, fp) != -1) {
        int r = 0, c = 0;
        if (sscanf(line, "%d %d", &r, &c) < 2) {
            fprintf(stderr, "Error: Failed to parse matrix dimensions\n");
            fclose(fp);
            return 1;
        }

        if (r != current_row) {
            // Finalize previous row
            if (current_row != -1) {
                if (buf_len > 0) {
                    neighbors[current_row] = (int*)malloc(sizeof(int) * buf_len);
                    for (int i = 0; i < buf_len; i++)
                        neighbors[current_row][i] = buf[i];
                    degree[current_row] = buf_len;
                    totalEntries += buf_len;
                } else {
                    neighbors[current_row] = NULL;
                    degree[current_row] = 0;
                }

                // Handle any skipped empty rows between current_row and r
                for (int rr = current_row + 1; rr < r; rr++) {
                    neighbors[rr] = NULL;
                    degree[rr] = 0;
                }

                buf_len = 0;
            }

            current_row = r;
        }

        // Instead of having a fixed-size buffer of number of nodes, we can dynamically resize as needed
        // Much more efficient for sparse graphs wiht a high number of nodes
        if (buf_len + 1 > buf_cap) {
            int new_cap = buf_cap == 0 ? 4 : buf_cap * 2;
            int* new_buf = (int*)realloc(buf, sizeof(int) * new_cap);
            if (!new_buf) {
                fprintf(stderr, "Memory allocation failed\n");
                free(buf);
                for (int i = 0; i < rows; i++)
                    free(neighbors[i]);
                free(neighbors);
                free(degree);
                fclose(fp);
                if (line)
                    free(line);
                return 1;
            }
            buf = new_buf;
            buf_cap = new_cap;
        }
        buf[buf_len++] = c;
    }

    // Finalize last seen row
    if (current_row != -1) {
        if (buf_len > 0) {
            neighbors[current_row] = (int*)malloc(sizeof(int) * buf_len);
            for (int i = 0; i < buf_len; i++)
                neighbors[current_row][i] = buf[i];
            degree[current_row] = buf_len;
            totalEntries += buf_len;
        } else {
            neighbors[current_row] = NULL;
            degree[current_row] = 0;
        }

        // Fill any remaining rows after last seen row with empty lists
        for (int rr = current_row + 1; rr < rows; rr++) {
            neighbors[rr] = NULL;
            degree[rr] = 0;
        }
    }

    graph->neighbors = neighbors;
    graph->degree = degree;
    graph->numberOfNodes = rows;
    graph->numberOfEdges = (int)totalEntries;

    free(buf);
    fclose(fp);
    if (line)
        free(line);

    return 0;
}