#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ModifiedValues
{
    int *modifiedIndex;
    int *modifiedValues;
    int length;
};

struct ReduceBuffers
{
    int *sendbuf;
    int *recvbuf;
    int *tempIndex;
    int *tempValues;
    int *mergedIndex;
    int *mergedValues;
    int capacity;
};

struct ReduceBuffers *allocReduceBuffers(int totalNodes)
{
    struct ReduceBuffers *buffers = malloc(sizeof(struct ReduceBuffers));
    buffers->capacity = totalNodes;
    buffers->sendbuf = malloc(sizeof(int) * (1 + 2 * totalNodes));
    buffers->recvbuf = malloc(sizeof(int) * (1 + 2 * totalNodes));
    buffers->tempIndex = malloc(sizeof(int) * totalNodes);
    buffers->tempValues = malloc(sizeof(int) * totalNodes);
    buffers->mergedIndex = malloc(sizeof(int) * totalNodes);
    buffers->mergedValues = malloc(sizeof(int) * totalNodes);
    return buffers;
}

void freeReduceBuffers(struct ReduceBuffers *buffers)
{
    free(buffers->sendbuf);
    free(buffers->recvbuf);
    free(buffers->tempIndex);
    free(buffers->tempValues);
    free(buffers->mergedIndex);
    free(buffers->mergedValues);
    free(buffers);
}

int countBit(int N)
{
    int count0 = 0;
    while (N > 0)
    {
        if (!(N & 1))
            count0++;
        else
        {
            return count0;
        }
        N = N >> 1;
    }
    return count0;
}

// void customReduce(struct ModifiedValues *values, int size, int rank, int totalNodes, struct ReduceBuffers *buffers)
// {
//     memcpy(buffers->mergedIndex, values->modifiedIndex, values->length * sizeof(int));
//     memcpy(buffers->mergedValues, values->modifiedValues, values->length * sizeof(int));
//     int mergedLength = values->length;

//     int step = 1;
//     while (step < size)
//     {
//         int dest = rank ^ step;

//         if (dest < size)
//         {
//             // Pack: length + indices + values
//             buffers->sendbuf[0] = mergedLength;
//             memcpy(buffers->sendbuf + 1, buffers->mergedIndex, mergedLength * sizeof(int));
//             memcpy(buffers->sendbuf + 1 + mergedLength, buffers->mergedValues, mergedLength * sizeof(int));

//             int send_size = 1 + 2 * mergedLength;
//             int recv_size;

//             // Exchange buffer sizes
//             MPI_Sendrecv(&send_size, 1, MPI_INT, dest, 0,
//                          &recv_size, 1, MPI_INT, dest, 0,
//                          MPI_COMM_WORLD, MPI_STATUS_IGNORE);

//             // Dynamically resize recvbuf if needed
//             if (recv_size > 1 + 2 * totalNodes)
//             {
//                 free(buffers->recvbuf);
//                 buffers->recvbuf = malloc(sizeof(int) * recv_size);
//             }

//             // Exchange actual data
//             MPI_Sendrecv(buffers->sendbuf, send_size, MPI_INT, dest, 1,
//                          buffers->recvbuf, recv_size, MPI_INT, dest, 1,
//                          MPI_COMM_WORLD, MPI_STATUS_IGNORE);

//             // Unpack received data
//             int recv_length = buffers->recvbuf[0];
//             int *recv_indices = buffers->recvbuf + 1;
//             int *recv_values = buffers->recvbuf + 1 + recv_length;

//             // Merge: sorted merge of two arrays
//             int i = 0, j = 0, k = 0;

//             while (i < mergedLength && j < recv_length)
//             {
//                 if (buffers->mergedIndex[i] < recv_indices[j])
//                 {
//                     buffers->tempIndex[k] = buffers->mergedIndex[i];
//                     buffers->tempValues[k] = buffers->mergedValues[i];
//                     i++;
//                     k++;
//                 }
//                 else if (buffers->mergedIndex[i] > recv_indices[j])
//                 {
//                     buffers->tempIndex[k] = recv_indices[j];
//                     buffers->tempValues[k] = recv_values[j];
//                     j++;
//                     k++;
//                 }
//                 else
//                 {
//                     // Same index: take minimum value
//                     buffers->tempIndex[k] = buffers->mergedIndex[i];
//                     buffers->tempValues[k] = (buffers->mergedValues[i] < recv_values[j])
//                                                  ? buffers->mergedValues[i]
//                                                  : recv_values[j];
//                     i++;
//                     j++;
//                     k++;
//                 }
//             }

//             // Copy remaining elements
//             while (i < mergedLength)
//             {
//                 buffers->tempIndex[k] = buffers->mergedIndex[i];
//                 buffers->tempValues[k] = buffers->mergedValues[i];
//                 i++;
//                 k++;
//             }
//             while (j < recv_length)
//             {
//                 buffers->tempIndex[k] = recv_indices[j];
//                 buffers->tempValues[k] = recv_values[j];
//                 j++;
//                 k++;
//             }

//             memcpy(buffers->mergedIndex, buffers->tempIndex, k * sizeof(int));
//             memcpy(buffers->mergedValues, buffers->tempValues, k * sizeof(int));
//             mergedLength = k;
//         }

//         step *= 2;
//     }

//     // Copy final result back to values
//     values->length = mergedLength;
//     memcpy(values->modifiedIndex, buffers->mergedIndex, mergedLength * sizeof(int));
//     memcpy(values->modifiedValues, buffers->mergedValues, mergedLength * sizeof(int));
// }

void customReduce(struct ModifiedValues *values, int size, int rank, int totalNodes, struct ReduceBuffers *buffers)
{
    memcpy(buffers->mergedIndex, values->modifiedIndex, values->length * sizeof(int));
    memcpy(buffers->mergedValues, values->modifiedValues, values->length * sizeof(int));
    int mergedLength = values->length;

    int step = 1;
    while (step < size)
    {
        int dest = rank ^ step;

        if (dest < size)
        {
            // Pack into a single buffer to avoid multiple MPI calls
            buffers->sendbuf[0] = mergedLength;
            memcpy(buffers->sendbuf + 1, buffers->mergedIndex, mergedLength * sizeof(int));
            memcpy(buffers->sendbuf + 1 + mergedLength, buffers->mergedValues, mergedLength * sizeof(int));

            int send_size = 1 + 2 * mergedLength;

            // Idea of Sendrecv: parallel MPI_Isend and MPI_Irecv (from src to dest), making send and recv proceed in parallel
            MPI_Status recv_status;
            MPI_Sendrecv(buffers->sendbuf, send_size, MPI_INT, dest, 1,
                         buffers->recvbuf, 1 + 2 * totalNodes, MPI_INT, dest, 1,
                         MPI_COMM_WORLD, &recv_status);

            int recv_size;
            MPI_Get_count(&recv_status, MPI_INT, &recv_size);

            // Unpack received data
            int recv_length = buffers->recvbuf[0];
            int *recv_indices = buffers->recvbuf + 1;
            int *recv_values = buffers->recvbuf + 1 + recv_length;

            // Merge received data with local data
            int i = 0, j = 0, k = 0;

            while (i < mergedLength && j < recv_length)
            {
                if (buffers->mergedIndex[i] < recv_indices[j])
                {
                    buffers->tempIndex[k] = buffers->mergedIndex[i];
                    buffers->tempValues[k] = buffers->mergedValues[i];
                    i++;
                    k++;
                }
                else if (buffers->mergedIndex[i] > recv_indices[j])
                {
                    buffers->tempIndex[k] = recv_indices[j];
                    buffers->tempValues[k] = recv_values[j];
                    j++;
                    k++;
                }
                else
                {
                    buffers->tempIndex[k] = buffers->mergedIndex[i];
                    buffers->tempValues[k] = (buffers->mergedValues[i] < recv_values[j])
                                                 ? buffers->mergedValues[i]
                                                 : recv_values[j];
                    i++;
                    j++;
                    k++;
                }
            }

            while (i < mergedLength)
            {
                buffers->tempIndex[k] = buffers->mergedIndex[i];
                buffers->tempValues[k] = buffers->mergedValues[i];
                i++;
                k++;
            }
            while (j < recv_length)
            {
                buffers->tempIndex[k] = recv_indices[j];
                buffers->tempValues[k] = recv_values[j];
                j++;
                k++;
            }

            memcpy(buffers->mergedIndex, buffers->tempIndex, k * sizeof(int));
            memcpy(buffers->mergedValues, buffers->tempValues, k * sizeof(int));
            mergedLength = k;
        }

        step *= 2;
    }

    values->length = mergedLength;
    memcpy(values->modifiedIndex, buffers->mergedIndex, mergedLength * sizeof(int));
    memcpy(values->modifiedValues, buffers->mergedValues, mergedLength * sizeof(int));
}