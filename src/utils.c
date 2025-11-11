#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int root(int vertex, int* parent);

void printGraph(struct Graph graph) {
    printf("Adjacency lists:\n");
    for (int i = 0; i < graph.numberOfNodes; i++) {
        printf("%d: ", i);
        for (int j = 0; j < graph.degree[i]; j++) {
            printf("%d, ", graph.neighbors[i][j]);
        }
        printf("\n");
    }
}

bool contains(int* parent, int length, int n) {
    for (int i = 0; i < length; i++) {
        if (parent[i] == n) {
            return true;
        }
    }
    return false;
}

int root(int vertex, int* parent) {
    // Find the topmost parent of vertex
    if (vertex == parent[vertex]) {
        return vertex;
    }

    parent[vertex] = root(parent[vertex], parent);
    return parent[vertex];
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
    free(uniqueParents);
}
