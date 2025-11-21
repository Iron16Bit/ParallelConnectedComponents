// https://cp-algorithms.com/data_structures/disjoint_set_union.html
// https://www.geeksforgeeks.org/dsa/number-of-connected-components-of-a-graph-using-disjoint-set-union/

#define _GNU_SOURCE
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "utils.c"
#include "readData.c"

struct SubGraph {
    int numberOfNodes;
    int numberOfEdges;
    int* degree;
    int** neighbors;
    int offset;
};

int* initParent(int length) {
    int* parent = malloc(sizeof(int) * length);

    // Initialize each parent as the node itself
    for (int i = 0; i < length; i++) {
        parent[i] = i;
    }

    return parent;
}

void findMinGrandparentOfNeighbours(struct SubGraph g, const int* gp, int* mngp) {
    for (int u = 0; u < g.numberOfNodes; u++) {
        int min_val = gp[g.offset + u];
        for (int k = 0; k < g.degree[u]; k++) {
            int v = g.neighbors[u][k];
            if (gp[v] < min_val)
                min_val = gp[v];
        }
        mngp[g.offset + u] = min_val;
    }
}

void vectorMin(int* dst, const int* src, int n, int offset) {
    for (int i = offset; i < offset + n; i++)
        if (src[i] < dst[i])
            dst[i] = src[i];
}

void computeGrandparent(const int* f, int* gp, int n, int offset) {
    for (int i = offset; i < offset + n; i++)
        gp[i] = f[f[i]];
}

void minGrandparent(int* f, const int* mngp, int n, int offset) {
    for (int u = offset; u < offset + n; u++) {
        int idx = f[u];
        if (mngp[u] < f[idx])
            f[idx] = mngp[u];
    }
}

bool converged(const int* gp, const int* dup, int n, int offset) {
    for (int i = offset; i < offset + n; i++)
        if (gp[i] != dup[i])
            return false;
    return true;
}

void update(int* dest, int* src, int n) {
    for (int i = 0; i < n; i++)
        dest[i] = src[i];
}

void connectedComponents(struct SubGraph graph, int* f, int rank, int* displacement, int* recvCounts, int totalNodes) {
    int n = graph.numberOfNodes;
    int* gp = malloc(sizeof(int) * totalNodes);
    int* dup = malloc(sizeof(int) * totalNodes);
    int* mngp = malloc(sizeof(int) * totalNodes);
    int* communicationBuffer = malloc(sizeof(int) * totalNodes);

    // Initialization
    for (int i = 0; i < totalNodes; i++) {
        gp[i] = f[i];
        dup[i] = gp[i];
        mngp[i] = gp[i];
        communicationBuffer[i] = f[i];
    }

    MPI_Request request;
    bool stop = false;
    bool local_stop = false;

    // Parameters for MPI_Iallgatherv
    int *sendcounts = recvCounts;
    int *displs = displacement;

    while (!stop) {
        MPI_Iallgatherv(f + graph.offset, n, MPI_INT, communicationBuffer, sendcounts, displs, MPI_INT, MPI_COMM_WORLD, &request);

        // 1a. mngp = A ⊗ gp  (neighbor-wise minimum grandparent)
        findMinGrandparentOfNeighbours(graph, gp, mngp);  // GrB mxv

        // 1b. Stochastic hooking: f[f[u]] = min(f[f[u]], mngp[u])
        minGrandparent(f, mngp, n, graph.offset);  // GrB assign

        // 2. Aggressive hooking: f[u] = min(f[u], mngp[u])
        vectorMin(f, mngp, n, graph.offset);  // GrB eWiseMult

        // 3. Shortcutting: f[u] = min(f[u], gp[u])
        vectorMin(f, gp, n, graph.offset);  // GrB eWiseMult

        // Merge results from all processes
        MPI_Wait(&request, MPI_STATUS_IGNORE);
        vectorMin(f, communicationBuffer, totalNodes, 0);

        // 4. Compute grandparent: gp[u] = f[f[u]]
        computeGrandparent(f, gp, n, graph.offset);

        // 5a. Check convergence
        local_stop = converged(gp, dup, n, graph.offset);
        int local_stop_int = local_stop ? 1 : 0;
        int stop_int = 0;
        MPI_Allreduce(&local_stop_int, &stop_int, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
        stop = (stop_int != 0);

        for (int i = 0; i < totalNodes; i++) {
            gp[i] = f[f[i]];
            dup[i] = gp[i];
            mngp[i] = gp[i];
        }
    }

    free(gp);
    free(dup);
    free(mngp);
    free(communicationBuffer);
}

void split(struct Graph graph, int size, int* sendCountsNode, int* sendCountsDegree) {
    int idealValuesPerProcess = (graph.numberOfEdges + size - 1) / size;
    int sentCount = 0;
    int totalSum = 0;

    for (int p = 0; p < size - 1; p++) {
        int sum = 0;
        int count = 0;
        for (int i = sentCount; i < graph.numberOfNodes; i++) {
            if (sum == 0) {
                sum = graph.degree[i];
                count += 1;
                sentCount++;
            } else {
                int newSum = sum + graph.degree[i];
                if (abs(newSum - idealValuesPerProcess) < abs(idealValuesPerProcess - sum)) {
                    sum = newSum;
                    sentCount++;
                    count += 1;
                } else {
                    break;
                }
            }
        }
        sendCountsNode[p] = count;
        sendCountsDegree[p] = sum;
        totalSum += sum;
    }
    // Last process
    sendCountsNode[size - 1] = graph.numberOfNodes - sentCount;
    sendCountsDegree[size - 1] = graph.numberOfEdges - totalSum;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Error Usage: %s <path_to_file> <number of runs>\n", argv[0]);
        exit(1);
    }
    int totalRuns = atoi(argv[2]);
    struct timeval startTime, endTime;
    double totalExecTime = 0;

    int rank, size;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (int iteration = 0; iteration < totalRuns; iteration++) {
        int* sendCountsNodes = (int*)malloc(sizeof(int) * size);
        int* sendCountsDegree = (int*)malloc(sizeof(int) * size);
        struct SubGraph g;

        if (rank == 0) {
            struct Graph graph;
            gettimeofday(&startTime, 0);
            initStruct(&graph, argv[1]);

            split(graph, size, sendCountsNodes, sendCountsDegree);

            // send info about each process subgraph
            MPI_Bcast(sendCountsNodes, size, MPI_INT, 0, MPI_COMM_WORLD);
            g.numberOfNodes = sendCountsNodes[rank];
            MPI_Scatter(sendCountsDegree, 1, MPI_INT, &g.numberOfEdges, 1, MPI_INT, 0, MPI_COMM_WORLD);

            // allocate resources to store subgraph
            int* flatNeighbor = (int*)malloc(sizeof(int) * g.numberOfEdges);
            g.degree = (int*)malloc(sizeof(int) * g.numberOfNodes);

            // receives data to insert in subgraph
            int* displsNeighbor = (int*)calloc(size, sizeof(int));
            int* displsDegree = (int*)calloc(size, sizeof(int));
            for (int i = 1; i < size; i++) {
                displsNeighbor[i] = displsNeighbor[i - 1] + sendCountsDegree[i - 1];
                displsDegree[i] = displsDegree[i - 1] + sendCountsNodes[i - 1];
            }
            MPI_Scatter(displsDegree, 1, MPI_INT, &g.offset, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Scatterv(graph.neighbors, sendCountsDegree, displsNeighbor, MPI_INT, flatNeighbor, g.numberOfEdges, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Scatterv(graph.degree, sendCountsNodes, displsDegree, MPI_INT, g.degree, g.numberOfNodes, MPI_INT, 0, MPI_COMM_WORLD);

            // de-flatten neighbors
            int flatNeighborIndex = 0;
            g.neighbors = (int**)malloc(sizeof(int*) * g.numberOfNodes);
            for (int i = 0; i < g.numberOfNodes; i++) {
                g.neighbors[i] = (int*)malloc(sizeof(int) * g.degree[i]);
                for (int k = 0; k < g.degree[i]; k++) {
                    g.neighbors[i][k] = flatNeighbor[flatNeighborIndex++];
                }
            }
        } else {
            // send info about each process subgraph
            MPI_Bcast(sendCountsNodes, size, MPI_INT, 0, MPI_COMM_WORLD);
            g.numberOfNodes = sendCountsNodes[rank];
            MPI_Scatter(sendCountsDegree, 1, MPI_INT, &g.numberOfEdges, 1, MPI_INT, 0, MPI_COMM_WORLD);

            // allocate resources to store subgraph
            int* flatNeighbor = (int*)malloc(sizeof(int) * g.numberOfEdges);
            g.degree = (int*)malloc(sizeof(int) * g.numberOfNodes);

            // receives data to insert in subgraph
            MPI_Scatter(NULL, 1, MPI_INT, &g.offset, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Scatterv(NULL, sendCountsDegree, NULL, MPI_INT, flatNeighbor, g.numberOfEdges, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Scatterv(NULL, sendCountsNodes, NULL, MPI_INT, g.degree, g.numberOfNodes, MPI_INT, 0, MPI_COMM_WORLD);
            // de-flatten neighbors
            int flatNeighborIndex = 0;
            g.neighbors = (int**)malloc(sizeof(int*) * g.numberOfNodes);
            for (int i = 0; i < g.numberOfNodes; i++) {
                g.neighbors[i] = (int*)malloc(sizeof(int) * g.degree[i]);
                for (int k = 0; k < g.degree[i]; k++) {
                    g.neighbors[i][k] = flatNeighbor[flatNeighborIndex++];
                }
            }
        }

        int* displs = malloc(sizeof(int) * size);
        displs[0] = 0;
        for (int i = 1; i < size; i++) {
            displs[i] = displs[i - 1] + sendCountsNodes[i - 1];
        }
        int totalNumberOfNodes = displs[size - 1] + sendCountsNodes[size - 1];

        int* parent = initParent(totalNumberOfNodes);

        connectedComponents(g, parent, rank, displs, sendCountsNodes, totalNumberOfNodes);

        if (rank == 0) {
            printSolution(parent, totalNumberOfNodes);
            gettimeofday(&endTime, 0);

            long executionSeconds = endTime.tv_sec - startTime.tv_sec;
            long executionMicroseconds = endTime.tv_usec - startTime.tv_usec;
            double elapsedTime = executionSeconds + executionMicroseconds * 1e-6;
            printf("[%d]\tExecution time: %.6fs\n", iteration, elapsedTime);
            totalExecTime += elapsedTime;
        }
    }
    if (rank == 0 && totalRuns != 1) {
        printf("Total execution time: %.6fs\n", totalExecTime);
        printf("Average execution time: %.6fs\n", totalExecTime / totalRuns);
    }
    MPI_Finalize();
    return 0;
}