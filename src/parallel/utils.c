#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

bool containsBinarySearch(int first, int last, int vertex, int* parent) {
    int middle = (first + last) / 2;
    while (first <= last) {
        if (parent[middle] < vertex)
            first = middle + 1;
        else if (parent[middle] == vertex) {
            return true;
        } else {
            last = middle - 1;
        }
        middle = (first + last) / 2;
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
        if (numberOfUniqueParents == 0 || !containsBinarySearch(0, numberOfUniqueParents, vertexRoot, uniqueParents)) {
            uniqueParents[numberOfUniqueParents] = vertexRoot;
            numberOfUniqueParents += 1;
        }
    }

    printf("Number of connected components: %d\n", numberOfUniqueParents);
    free(uniqueParents);
}
