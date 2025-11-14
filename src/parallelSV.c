// https://cp-algorithms.com/data_structures/disjoint_set_union.html
// https://www.geeksforgeeks.org/dsa/number-of-connected-components-of-a-graph-using-disjoint-set-union/

#define _GNU_SOURCE
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "readData.c"
#include "utils.c"

int *initParent(struct Graph graph) {
    int length = graph.numberOfNodes;
    int *parent = malloc(sizeof(int) * length);

    // Initialize each parent as the node itself
    for (int i = 0; i < length; i++) {
        parent[i] = i;
    }

    return parent;
}

void findMinGrandparentOfNeighbours(struct Graph g, const int *gp, int *mngp) {
    for (int u = 0; u < g.numberOfNodes; u++) {
        int min_val = gp[u];
        for (int k = 0; k < g.degree[u]; k++) {
            int v = g.neighbors[u][k];
            if (gp[v] < min_val)
                min_val = gp[v];
        }
        mngp[u] = min_val;
    }
}

void vectorMin(int *dst, const int *src, int n) {
    for (int i = 0; i < n; i++)
        if (src[i] < dst[i])
            dst[i] = src[i];
}

void computeGrandparent(const int *f, int *gp, int n) {
    for (int i = 0; i < n; i++)
        gp[i] = f[f[i]];
}

void minGrandparent(int *f, const int *mngp, int n) {
    for (int u = 0; u < n; u++) {
        int idx = f[u];
        if (mngp[u] < f[idx])
            f[idx] = mngp[u];
    }
}

bool converged(const int *gp, const int *dup, int n) {
    for (int i = 0; i < n; i++)
        if (gp[i] != dup[i])
            return false;
    return true;
}

void connectedComponents(struct Graph graph, int *f) {
    int n = graph.numberOfNodes;
    int *gp = malloc(sizeof(int) * n);
    int *dup = malloc(sizeof(int) * n);
    int *mngp = malloc(sizeof(int) * n);

    // Initialization
    for (int i = 0; i < n; i++) {
        gp[i] = f[i];
        dup[i] = gp[i];
        mngp[i] = gp[i];
    }

    bool stop = false;
    while (!stop) {
        // 1a. mngf = A ⊗ gf  (neighbor-wise minimum grandparent)
        findMinGrandparentOfNeighbours(graph, gp, mngp);  // GrB mxv

        // 1b. Stochastic hooking: f[f[u]] = min(f[f[u]], mngf[u])
        minGrandparent(f, mngp, n);  // GrB assign

        // 2. Aggressive hooking: f[u] = min(f[u], mngf[u])
        vectorMin(f, mngp, n);  // GrB eWiseMult

        // 3. Shortcutting: f[u] = min(f[u], gf[u])
        vectorMin(f, gp, n);  // GrB eWiseMult

        // 4. Compute grandparent: gf[u] = f[f[u]]
        computeGrandparent(f, gp, n);

        // 5a. Check convergence
        stop = converged(gp, dup, n);

        // 5b. Update dup = gf
        for (int i = 0; i < n; i++)  // GrB assign
            dup[i] = gp[i];
    }

    free(gp);
    free(dup);
    free(mngp);
}

int *split(struct Graph graph, int size) {
    int idealValuesPerProcess = (graph.numberOfEdges + size - 1) / size;
    int *sendCounts = (int *)malloc(sizeof(int) * size);
    int sentCount = 0;
    // ultimo processo processato dopo
    for (int p = 0; p < size - 1; p++) {
        int sum = 0;
        for (int i = sentCount; i < graph.numberOfNodes; i++) {
            if (sum == 0) {
                sum = graph.degree[i];
                sentCount++;
            } else {
                int newSum = sum + graph.degree[i];
                if (abs(newSum - idealValuesPerProcess) < abs(idealValuesPerProcess - sum)) {
                    sum = newSum;
                    sentCount++;
                } else {
                    break;
                }
            }
        }
        sendCounts[p] = sentCount-1;
    }
    // ultimo processo si accolla quello che resta senza fare domande
    sendCounts[size-1] = graph.numberOfNodes;
    return sendCounts;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error Usage: ./a.out <path_to_file>\n");
        exit(1);
    }

    struct Graph graph;
    struct timeval startTime, endTime;
    int rank, size;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        gettimeofday(&startTime, 0);
        initStruct(&graph, argv[1]);
        // printGraph(graph);
        int *sendCounts = split(graph, size);
        printf("sendCounnts:\n");
        for (int i = 0; i < size; i++) {
            printf("%d\t", sendCounts[i]);
        }
        printf("\n");
    } else {
    }

    // int *parent = initParent(graph);
    // connectedComponents(graph, parent);

    // if (rank == 0) {
    //     printSolution(parent, graph.numberOfNodes);
    //     gettimeofday(&endTime, 0);

    //     long executionSeconds = endTime.tv_sec - startTime.tv_sec;
    //     long executionMicroseconds = endTime.tv_usec - startTime.tv_usec;
    //     double elapsedTime = executionSeconds + executionMicroseconds * 1e-6;
    //     printf("Execution time: %.6fs\n", elapsedTime);
    // }
    MPI_Finalize();
    return 0;
}