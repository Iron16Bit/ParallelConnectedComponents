// https://cp-algorithms.com/data_structures/disjoint_set_union.html
// https://www.geeksforgeeks.org/dsa/number-of-connected-components-of-a-graph-using-disjoint-set-union/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "readData.c"
#include "utils.c"

int *initParent(struct Graph graph)
{
    int length = graph.numberOfNodes;
    int *parent = malloc(sizeof(int) * length);

    // Initialize each parent as the node itself
    for (int i = 0; i < length; i++)
    {
        parent[i] = i;
    }

    return parent;
}

int root(int vertex, int *parent)
{
    // Find the topmost parent of vertex
    if (vertex == parent[vertex])
    {
        return vertex;
    }

    parent[vertex] = root(parent[vertex], parent);
    return parent[vertex];
}

void connectedComponents(struct Graph graph, int *parent, int *rank)
{
    // Connect nodes using adjacency lists
    for (int u = 0; u < graph.numberOfNodes; u++)
    {
        for (int k = 0; k < graph.degree[u]; k++)
        {
            int v = graph.neighbors[u][k];
            if (u == v)
                continue; // skip edges to self
            int vertexA = root(u, parent);
            int vertexB = root(v, parent);

            if (vertexA != vertexB)
            {
                if (rank[vertexA] < rank[vertexB])
                {
                    swap(&vertexA, &vertexB);
                }
                parent[vertexB] = vertexA;
                if (rank[vertexA] == rank[vertexB])
                {
                    rank[vertexA] += 1;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Error Usage: ./a.out <path_to_file>\n");
        exit(1);
    }

    struct timeval startTime, endTime;
    struct timezone tz;

    gettimeofday(&startTime, &tz);

    struct Graph graph;
    initStruct(&graph, argv[1]);

    // printGraph(graph);

    int *parent = initParent(graph);
    int *rank = calloc(graph.numberOfNodes, sizeof(int));
    connectedComponents(graph, parent, rank);

    printSolution(parent, graph.numberOfNodes);
    gettimeofday(&endTime, &tz);

    long unsigned int elapsedTime = (endTime.tv_sec - startTime.tv_sec);
    printf("Execution time: %lus\n", elapsedTime);

    return 0;
}