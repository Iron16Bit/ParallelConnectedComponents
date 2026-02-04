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

// Rem's algorithm
void findCommonAncestor(int x, int y, int *parent)
{
    int rootX = x, rootY = y;
    int tmp;
    while (parent[rootX] != parent[rootY])
    {
        if (parent[rootX] < parent[rootY])
        {
            if (rootX == parent[rootX])
            {
                parent[rootX] = parent[rootY];
                break;
            }
            tmp = parent[rootX];
            parent[rootX] = parent[rootY];
            rootX = tmp;
        }
        else
        {
            if (rootY == parent[rootY])
            {
                parent[rootY] = parent[rootX];
                break;
            }
            tmp = parent[rootY];
            parent[rootY] = parent[rootX];
            rootY = tmp;
        }
    }
}

void connectedComponents(struct Graph graph, int *parent)
{
    int *parentNext = initParent(graph);
    bool stop = false;
    while (!stop)
    {
        stop = true;
        // tree hooking
        // stochastic
        for (int u = 0; u < graph.numberOfNodes; u++)
        {
            for (int k = 0; k < graph.degree[u]; k++)
            {
                int v = graph.neighbors[u][k];
                if (parentNext[parent[u]] < parent[parent[v]])
                {
                    parentNext[parent[u]] = parent[parent[v]];
                }
            }
        }

        // shortcutting
        for (int u = 0; u < graph.numberOfNodes; u++)
        {
            if (parentNext[u] < parent[parent[u]])
            {
                parentNext[u] = parent[parent[u]];
            }
        }

        for (int i = 0; i < graph.numberOfNodes; i++)
        {
            if (parent[parent[i]] != parentNext[parentNext[i]])
            {
                stop = false;
            }
            parent[i] = parentNext[i];
        }
    }
}

void printTime(char *msg, struct timeval startTime, struct timeval endTime)
{
    long executionSeconds = endTime.tv_sec - startTime.tv_sec;
    long executionMicroseconds = endTime.tv_usec - startTime.tv_usec;
    double elapsedTime = executionSeconds + executionMicroseconds * 1e-6;
    printf("%s time: %.6fs\n", msg, elapsedTime);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Error Usage: ./a.out <path_to_file> <num_executions>\n");
        exit(1);
    }

    int numExecutions = atoi(argv[2]);
    if (numExecutions <= 0)
    {
        fprintf(stderr, "Error: Number of executions must be positive\n");
        exit(1);
    }

    struct timeval startTime, afterIOTime, beforeComputation, afterComputation, endTime;
    double totalExecutionTime = 0.0;
    double totalComputationTime = 0.0;

    for (int exec = 0; exec < numExecutions; exec++)
    {
        gettimeofday(&startTime, 0);

        struct Graph graph;
        initStruct(&graph, argv[1]);
        gettimeofday(&afterIOTime, 0);

        int *parent = initParent(graph);
        gettimeofday(&beforeComputation, 0);
        connectedComponents(graph, parent);
        gettimeofday(&afterComputation, 0);

        if (exec == 0)
        {
            printSolution(parent, graph.numberOfNodes);
        }
        gettimeofday(&endTime, 0);

        if (exec == 0)
        {
            printTime("IO", startTime, afterIOTime);
            printTime("Computation", beforeComputation, afterComputation);
            printTime("Solution reconstruction", afterComputation, endTime);
        }

        long executionSeconds = endTime.tv_sec - startTime.tv_sec;
        long executionMicroseconds = endTime.tv_usec - startTime.tv_usec;
        double elapsedTime = executionSeconds + executionMicroseconds * 1e-6;

        long computationSeconds = afterComputation.tv_sec - beforeComputation.tv_sec;
        long computationMicroseconds = afterComputation.tv_usec - beforeComputation.tv_usec;
        double computationTime = computationSeconds + computationMicroseconds * 1e-6;

        totalExecutionTime += elapsedTime;
        totalComputationTime += computationTime;

        free(parent);
        free(graph.degree);
        for(int i = 0; i < graph.numberOfNodes; i++) {
            free(graph.neighbors[i]);
        }
        free(graph.neighbors);
    }

    printf("\nAverage execution time: %.6fs\n", totalExecutionTime / numExecutions);
    printf("Average computation time: %.6fs\n", totalComputationTime / numExecutions);

    return 0;
}