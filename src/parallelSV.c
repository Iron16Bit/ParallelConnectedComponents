// https://cp-algorithms.com/data_structures/disjoint_set_union.html
// https://www.geeksforgeeks.org/dsa/number-of-connected-components-of-a-graph-using-disjoint-set-union/

#define _GNU_SOURCE
#include <mpi.h>
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

void findMinGrandparentOfNeighbours(struct Graph g, const int *gp, int *mngp, int start, int end)
{
    for (int u = start; u < end; u++)
    {
        int min_val = gp[u];
        for (int k = 0; k < g.degree[u]; k++)
        {
            int v = g.neighbors[u][k];
            if (gp[v] < min_val)
                min_val = gp[v];
        }
        mngp[u] = min_val;
    }
}

void vectorMin(int *dst, const int *src, int start, int end)
{
    for (int i = start; i < end; i++)
        if (src[i] < dst[i])
            dst[i] = src[i];
}

void computeGrandparent(const int *f, int *gp, int start, int end)
{
    for (int i = start; i < end; i++)
        gp[i] = f[f[i]];
}

void minGrandparent(int *f, const int *mngp, int start, int end)
{
    for (int u = start; u < end; u++)
    {
        int idx = f[u];
        if (mngp[u] < f[idx])
            f[idx] = mngp[u];
    }
}

bool converged(const int *gp, const int *dup, int start, int end)
{
    for (int i = start; i < end; i++)
        if (gp[i] != dup[i])
            return false;
    return true;
}

void update(int *dest, int *src, int start, int end)
{
    for (int i = start; i < end; i++)
        dest[i] = src[i];
}

void connectedComponents(struct Graph graph, int *f, int rank, int *displacement, int *recvCounts)
{
    int n = graph.numberOfNodes;
    int *gp = malloc(sizeof(int) * n);
    int *dup = malloc(sizeof(int) * n);
    int *mngp = malloc(sizeof(int) * n);

    int start = displacement[rank];
    int end = start + recvCounts[rank];

    // Initialization
    for (int i = 0; i < n; i++)
    {
        gp[i] = f[i];
        dup[i] = gp[i];
        mngp[i] = gp[i];
    }

    bool stop = false;
    bool local_stop = false;
    while (!stop)
    {
        // 1a. mngp = A ⊗ gp  (neighbor-wise minimum grandparent)
        findMinGrandparentOfNeighbours(graph, gp, mngp, start, end); // GrB mxv

        // 1b. Stochastic hooking: f[f[u]] = min(f[f[u]], mngp[u])
        minGrandparent(f, mngp, start, end); // GrB assign

        // 2. Aggressive hooking: f[u] = min(f[u], mngp[u])
        vectorMin(f, mngp, start, end); // GrB eWiseMult

        // 3. Shortcutting: f[u] = min(f[u], gp[u])
        vectorMin(f, gp, start, end); // GrB eWiseMult

        // 4. Compute grandparent: gp[u] = f[f[u]]
        computeGrandparent(f, gp, start, end);

        // 5a. Check convergence
        local_stop = converged(gp, dup, start, end);

        // 5b. Update dup = gp
        update(dup, gp, start, end); // GrB assign

        int local_stop_int = local_stop ? 1 : 0;
        int stop_int = 0;

        MPI_Allgatherv(f + start, recvCounts[rank], MPI_INT, f, recvCounts, displacement, MPI_INT, MPI_COMM_WORLD);
        MPI_Allgatherv(gp + start, recvCounts[rank], MPI_INT, gp, recvCounts, displacement, MPI_INT, MPI_COMM_WORLD);
        MPI_Allgatherv(dup + start, recvCounts[rank], MPI_INT, dup, recvCounts, displacement, MPI_INT, MPI_COMM_WORLD);
        MPI_Allgatherv(mngp + start, recvCounts[rank], MPI_INT, mngp, recvCounts, displacement, MPI_INT, MPI_COMM_WORLD);

        MPI_Allreduce(&local_stop_int, &stop_int, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
        stop = (stop_int != 0);
    }

    free(gp);
    free(dup);
    free(mngp);
}

int *split(struct Graph graph, int size)
{
    int idealValuesPerProcess = (graph.numberOfEdges + size - 1) / size;
    int *sendCounts = (int *)malloc(sizeof(int) * size);
    int sentCount = 0;

    for (int p = 0; p < size - 1; p++)
    {
        int sum = 0;
        for (int i = sentCount; i < graph.numberOfNodes; i++)
        {
            if (sum == 0)
            {
                sum = graph.degree[i];
                sentCount++;
            }
            else
            {
                int newSum = sum + graph.degree[i];
                if (abs(newSum - idealValuesPerProcess) < abs(idealValuesPerProcess - sum))
                {
                    sum = newSum;
                    sentCount++;
                }
                else
                {
                    break;
                }
            }
        }
        sendCounts[p] = sentCount;
    }
    // Last process
    sendCounts[size - 1] = graph.numberOfNodes;
    return sendCounts;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Error Usage: ./a.out <path_to_file>\n");
        exit(1);
    }

    struct Graph graph;
    struct timeval startTime, endTime;
    int rank, size;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
    {
        gettimeofday(&startTime, 0);
    }
    initStruct(&graph, argv[1]);
    // printGraph(graph);
    int *sendCounts = split(graph, size);
    int *recvCounts = malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++)
    {
        int start = i == 0 ? 0 : sendCounts[i - 1];
        int end = sendCounts[i];
        recvCounts[i] = end - start;
    }

    int *displs = malloc(sizeof(int) * size);
    displs[0] = 0;
    for (int i = 1; i < size; i++)
    {
        displs[i] = displs[i - 1] + recvCounts[i - 1];
    }

    int *parent = initParent(graph);

    connectedComponents(graph, parent, rank, displs, recvCounts);

    if (rank == 0)
    {
        printSolution(parent, graph.numberOfNodes);
        gettimeofday(&endTime, 0);

        long executionSeconds = endTime.tv_sec - startTime.tv_sec;
        long executionMicroseconds = endTime.tv_usec - startTime.tv_usec;
        double elapsedTime = executionSeconds + executionMicroseconds * 1e-6;
        printf("Execution time: %.6fs\n", elapsedTime);
    }
    MPI_Finalize();
    return 0;
}