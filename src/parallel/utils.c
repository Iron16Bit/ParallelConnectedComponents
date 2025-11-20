#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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
        // int vertexRoot = root(i, parent);
        int vertexRoot = parent[i];
        if (!contains(uniqueParents, numberOfUniqueParents, vertexRoot)) {
            uniqueParents[numberOfUniqueParents] = vertexRoot;
            numberOfUniqueParents += 1;
        }
    }

    printf("Number of connected components: %d\n", numberOfUniqueParents);
    free(uniqueParents);
}
