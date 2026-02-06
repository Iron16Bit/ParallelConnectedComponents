// https://cp-algorithms.com/data_structures/disjoint_set_union.html
// https://www.geeksforgeeks.org/dsa/number-of-connected-components-of-a-graph-using-disjoint-set-union/

#define _GNU_SOURCE
#include <mpi.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#include "readData.c"
#include "utils.c"

int thread_count = 1;

#ifndef SYNC_BATCH_K
#define SYNC_BATCH_K 4
#endif

//Graph structure of each process
struct SubGraph
{
    int numberOfNodes;      // Number of nodes in the subgraph 
    int numberOfEdges;      // Number of edges in the subgraph
    int *degree;            // Degree of each node
    int **neighbors;        // Adjacency list 
    int offset;             // Global node index offset
};

// Initialize the "parent" array, setting each node as the parent of itself.
int *initParent(int length)
{
    int *parent = malloc(sizeof(int) * length);
    for (int i = 0; i < length; i++)
        parent[i] = i;
    return parent;
}

/* Neighbor-wise propagation: mngp ← graph ⊗ gp
* in  | struct SubGraph g: Working graph of process 
* in  | const int* gp: array of grandparents
* out | int* mngp: array of the minimum grandparents
*/
void findMinGrandparentOfNeighbours(struct SubGraph g, const int *gp, int *mngp)
{
#pragma omp for
    for (int u = 0; u < g.numberOfNodes; u++)
    {
        int min_val = gp[g.offset + u];
        for (int k = 0; k < g.degree[u]; k++)
        {
            int v = g.neighbors[u][k];
            if (gp[v] < min_val)
                min_val = gp[v];
        }
        mngp[g.offset + u] = min_val;
    }
}

/* Find minimum values between src1 and src2 and stores it into dst.
* out | int *dst: destination array
* in  | const int *src1: first source
* in  | const int *src2: second source, to compare
* in  | int n: number of elements to compare
* in  | int offset: starting index
*/
void vectorMin(int *dst, const int *src1, const int *src2, int n, int offset)
{
#pragma omp for
    for (int i = offset; i < offset + n; i++)
    {
        if (src1[i] < dst[i])
            dst[i] = src1[i];
        else if (src2[i] < dst[i])
            dst[i] = src2[i];
    }
}

/* Check if there is a difference between gp and dup. If there isn't returns 0, otherwise 1
* in  | const int* gp: first array
* in  | const int* dup: second array
* in  | int n: number of elements to compare
* in  | int offset: starting index
*/
int converged(const int *gp, const int *dup, int n, int offset)
{
    for (int i = offset; i < offset + n; i++)
        if (gp[i] != dup[i])
            return 1; 
    return 0;
}

/* Find connected Components
* in  | struct SubGraph g: Working graph of process
* out | int* f: Parent array
* in  | int rank: MPI rank of the process
* in  | int* displacement: Array of the offset of each process. Meaning, where each process starts to work on the total graph
* in  | int* recvCounts: Array of the numbers of nodes to send to each process.
* in  | int totalNodes: total numbers of nodes in the whole Graph
*/
void connectedComponents(struct SubGraph graph, int *f, int rank, int *displacement, int *recvCounts, int totalNodes)
{
    int n = graph.numberOfNodes;

    int *gp = malloc(sizeof(int) * totalNodes);
    int *dup = malloc(sizeof(int) * totalNodes);
    int *mngp = malloc(sizeof(int) * totalNodes);
    int *communicationBuffer = malloc(sizeof(int) * totalNodes);

    // Parameters for MPI_Allgatherv/Iallgatherv
    int *sendcounts = recvCounts;
    int *displs = displacement;

    // Initialization
    #pragma omp parallel for
    for (int i = 0; i < totalNodes; i++)
    {
        gp[i] = f[i];
        dup[i] = gp[i];
        mngp[i] = gp[i];
        communicationBuffer[i] = f[i];
    }

    int global_sum = 1;

    #pragma omp parallel num_threads(thread_count)
    {
        int local_diff = 1;
        int reduce_result = 1;

        while (global_sum != 0)
        {
            for (int step = 0; step < SYNC_BATCH_K; step++)
            {
                // 1a. mngp = A ⊗ gp
                findMinGrandparentOfNeighbours(graph, gp, mngp);

                // 1b. Stochastic hooking: f[f[u]] = min(f[f[u]], mngp[u])
                #pragma omp for schedule(static)
                for (int u = graph.offset; u < graph.offset + n; u++)
                {
                    int idx = f[u];
                    int old_val, new_val;
                    do
                    {
                        old_val = f[idx];
                        new_val = (mngp[u] < old_val) ? mngp[u] : old_val;
                    } while (!__sync_bool_compare_and_swap(&f[idx], old_val, new_val) && new_val != old_val);
                }

                // 2. Aggressive hooking + 3. Shortcutting
                vectorMin(f, mngp, gp, n, graph.offset);

                // MPI synchronization
                if (step == SYNC_BATCH_K - 1) {
                    #pragma omp master
                    {
                        MPI_Allgatherv(f + graph.offset, n, MPI_INT,
                                        communicationBuffer, sendcounts, displs, MPI_INT,
                                        MPI_COMM_WORLD);
                        memcpy(f, communicationBuffer, totalNodes * sizeof(int));
                    }
                }

                // 4. Compute grandparent: gp[u] = f[f[u]]
                #pragma omp for schedule(static)
                for (int i = graph.offset; i < graph.offset + n; i++)
                {
                    gp[i] = f[f[i]] > communicationBuffer[f[i]] ? communicationBuffer[f[i]] : f[f[i]];
                }
                
                // 5a. Check convergence
                if (step == SYNC_BATCH_K - 1) {
                    #pragma omp master
                    {
                        local_diff = converged(gp, dup, n, graph.offset);
                        MPI_Allreduce(&local_diff, &reduce_result, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
                        global_sum = reduce_result;
                    }
                    #pragma omp barrier
                } else {
                    #pragma omp master
                    {
                        global_sum = 1;
                    }
                    #pragma omp barrier
                }

                // 5b. Update dup for next iteration
                #pragma omp for schedule(static)
                for (int i = graph.offset; i < graph.offset + n; i++)
                {
                    gp[i] = f[f[i]];
                    dup[i] = gp[i];
                }
            }
        }
    }

    free(gp);
    free(dup);
    free(mngp);
    free(communicationBuffer);
}

/* Calculates how to split the graph into smaller subgraphs, for each process to work on.
* in  | struct Graph graph: Whole graph
* in  | int size: MPI size, number of process to split the Graph for
* out  | int* sendCountsNode: amount of nodes of each subgraph
* out  | int* sendCountsDegree: total amount of the degrees of each subgprah
*/
void split(struct Graph graph, int size, int *sendCountsNode, int *sendCountsDegree)
{
    int idealValuesPerProcess = (graph.numberOfEdges + size - 1) / size;
    int sentCount = 0;
    int totalSum = 0;

    for (int p = 0; p < size - 1; p++)
    {
        int sum = 0;
        int count = 0;
        for (int i = sentCount; i < graph.numberOfNodes; i++)
        {
            if (sum == 0)
            {
                sum = graph.degree[i];
                count += 1;
                sentCount++;
            }
            else
            {
                int newSum = sum + graph.degree[i];
                if (abs(newSum - idealValuesPerProcess) < abs(idealValuesPerProcess - sum))
                {
                    sum = newSum;
                    sentCount++;
                    count += 1;
                }
                else
                {
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

/* Print delta time with a relevant message
* in  | char *msg: message to print
* in  | struct timeval startTime: start time of the operation to time
* in  | struct timeval endTime: end time of the operation to time
*/
void printTime(char *msg, struct timeval startTime, struct timeval endTime)
{
    long executionSeconds = endTime.tv_sec - startTime.tv_sec;
    long executionMicroseconds = endTime.tv_usec - startTime.tv_usec;
    double elapsedTime = executionSeconds + executionMicroseconds * 1e-6;
    printf("%s time: %.6fs\n", msg, elapsedTime);
}

int main(int argc, char *argv[])
{
    if (argc != 4 && argc != 3)
    {
        fprintf(stderr, "Error Usage: %s <path_to_file> <number of threads> <number of runs>\n", argv[0]);
        exit(1);
    }

    int totalRuns = 1;
    if (argc == 4)
        totalRuns = atoi(argv[3]);
    thread_count = atoi(argv[2]);

    struct timeval startTime, afterIOTime, beforeComputation, afterComputation, endTime;
    double totalExecTime = 0;
    double totalComputationTime = 0;

    int rank, size;

    int provided = 0;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    if (provided < MPI_THREAD_FUNNELED)
    {
        fprintf(stderr, "Error: MPI does not provide required thread support (need MPI_THREAD_FUNNELED, got %d)\n", provided);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (int iteration = 0; iteration < totalRuns; iteration++)
    {
        int *sendCountsNodes = (int *)malloc(sizeof(int) * size);
        int *sendCountsDegree = (int *)malloc(sizeof(int) * size);
        struct SubGraph g;

        if (rank == 0)
        {
            struct Graph graph;
            gettimeofday(&startTime, 0);
            initStruct(&graph, argv[1]);
            gettimeofday(&afterIOTime, 0);

            split(graph, size, sendCountsNodes, sendCountsDegree);

            // send info about each process subgraph
            MPI_Bcast(sendCountsNodes, size, MPI_INT, 0, MPI_COMM_WORLD);
            g.numberOfNodes = sendCountsNodes[rank];
            MPI_Scatter(sendCountsDegree, 1, MPI_INT, &g.numberOfEdges, 1, MPI_INT, 0, MPI_COMM_WORLD);

            // allocate resources to store subgraph
            int *flatNeighbor = (int *)malloc(sizeof(int) * g.numberOfEdges);
            g.degree = (int *)malloc(sizeof(int) * g.numberOfNodes);

            // receives data to insert in subgraph
            int *displsNeighbor = (int *)calloc(size, sizeof(int));
            int *displsDegree = (int *)calloc(size, sizeof(int));
            for (int i = 1; i < size; i++)
            {
                displsNeighbor[i] = displsNeighbor[i - 1] + sendCountsDegree[i - 1];
                displsDegree[i] = displsDegree[i - 1] + sendCountsNodes[i - 1];
            }
            MPI_Scatter(displsDegree, 1, MPI_INT, &g.offset, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Scatterv(graph.neighbors, sendCountsDegree, displsNeighbor, MPI_INT,
                         flatNeighbor, g.numberOfEdges, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Scatterv(graph.degree, sendCountsNodes, displsDegree, MPI_INT,
                         g.degree, g.numberOfNodes, MPI_INT, 0, MPI_COMM_WORLD);

            // de-flatten neighbors
            int flatNeighborIndex = 0;
            g.neighbors = (int **)malloc(sizeof(int *) * g.numberOfNodes);
            for (int i = 0; i < g.numberOfNodes; i++)
            {
                g.neighbors[i] = (int *)malloc(sizeof(int) * g.degree[i]);
                for (int k = 0; k < g.degree[i]; k++)
                    g.neighbors[i][k] = flatNeighbor[flatNeighborIndex++];
            }
            free(flatNeighbor);
        }
        else
        {
            // send info about each process subgraph
            MPI_Bcast(sendCountsNodes, size, MPI_INT, 0, MPI_COMM_WORLD);
            g.numberOfNodes = sendCountsNodes[rank];
            MPI_Scatter(sendCountsDegree, 1, MPI_INT, &g.numberOfEdges, 1, MPI_INT, 0, MPI_COMM_WORLD);

            // allocate resources to store subgraph
            int *flatNeighbor = (int *)malloc(sizeof(int) * g.numberOfEdges);
            g.degree = (int *)malloc(sizeof(int) * g.numberOfNodes);

            // receives data to insert in subgraph
            MPI_Scatter(NULL, 1, MPI_INT, &g.offset, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Scatterv(NULL, sendCountsDegree, NULL, MPI_INT,
                         flatNeighbor, g.numberOfEdges, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Scatterv(NULL, sendCountsNodes, NULL, MPI_INT,
                         g.degree, g.numberOfNodes, MPI_INT, 0, MPI_COMM_WORLD);

            // de-flatten neighbors
            int flatNeighborIndex = 0;
            g.neighbors = (int **)malloc(sizeof(int *) * g.numberOfNodes);
            for (int i = 0; i < g.numberOfNodes; i++)
            {
                g.neighbors[i] = (int *)malloc(sizeof(int) * g.degree[i]);
                for (int k = 0; k < g.degree[i]; k++)
                    g.neighbors[i][k] = flatNeighbor[flatNeighborIndex++];
            }
            free(flatNeighbor);
        }

        int *displs = malloc(sizeof(int) * size);
        displs[0] = 0;
        for (int i = 1; i < size; i++)
            displs[i] = displs[i - 1] + sendCountsNodes[i - 1];

        int totalNumberOfNodes = displs[size - 1] + sendCountsNodes[size - 1];
        int *parent = initParent(totalNumberOfNodes);

        gettimeofday(&beforeComputation, 0);
        connectedComponents(g, parent, rank, displs, sendCountsNodes, totalNumberOfNodes);
        gettimeofday(&afterComputation, 0);

        if (rank == 0)
        {
            printSolution(parent, totalNumberOfNodes);
            gettimeofday(&endTime, 0);

            printTime("IO", startTime, afterIOTime);
            printTime("Computation", beforeComputation, afterComputation);
            printTime("Solution reconstruction", afterComputation, endTime);

            long executionSeconds = endTime.tv_sec - startTime.tv_sec;
            long executionMicroseconds = endTime.tv_usec - startTime.tv_usec;
            double elapsedTime = executionSeconds + executionMicroseconds * 1e-6;
            printf("[%d]\tExecution time: %.6fs\n", iteration, elapsedTime);
            totalExecTime += elapsedTime;

            executionSeconds = afterComputation.tv_sec - beforeComputation.tv_sec;
            executionMicroseconds = afterComputation.tv_usec - beforeComputation.tv_usec;
            totalComputationTime += (executionSeconds + executionMicroseconds * 1e-6);
        }

        free(sendCountsNodes);
        free(sendCountsDegree);
        free(displs);
        free(parent);

        free(g.degree);
        for( int i = 0; i < g.numberOfNodes; i++)
            free(g.neighbors[i]);
        free(g.neighbors);
    }

    if (rank == 0 && totalRuns != 1)
    {
        printf("Total execution time: %.6fs\n", totalExecTime);
        printf("Average execution time: %.6fs\n", totalExecTime / totalRuns);
        printf("Average computation time: %.6fs\n", totalComputationTime / totalRuns);
    }

    MPI_Finalize();
    return 0;
}