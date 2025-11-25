#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int IsPowerOfTwo(int x) {
    return (x & (x - 1)) == 0;
}

int countBit(int N) {
    int count0 = 0;
    while (N > 0) {
        if (!(N & 1))
            count0++;
        else {
            return count0;
        }
        N = N >> 1;
    }
    return count0;
}

void customReduce(int* modifiedIndex, int* modifiedValue, int length, MPI_Datatype datatype, int size, int rank, int totalNodes) {
    int* bufferIndex = (int*)calloc(totalNodes, sizeof(int));
    int* bufferValue = (int*)calloc(totalNodes, sizeof(int));

    if (rank == 0) {
        int maxArch = floor(logf(size)) + 1;
        for (int i = 0; i < totalNodes; i++) {
            if (modifiedIndex[i] == -1) {
                bufferIndex[i] = -1;
                bufferValue[i] = -1;
                break;
            }
            bufferIndex[i] = modifiedIndex[i];
            bufferValue[i] = modifiedValue[i];
        }
        for (int i = 0; i < maxArch; i++) {
            MPI_Recv(modifiedIndex, totalNodes, datatype, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = 0; i < totalNodes; i++) {
                if (modifiedIndex[i] == -1) {
                    break;
                }
                for (int j = 0; j < totalNodes; j++) {
                    if (bufferIndex[j] > modifiedIndex[i]) {
                        // Move to the right
                        for (int k = totalNodes - 1; k >= j; k--) {
                            modifiedIndex[k] = modifiedIndex[k - 1];
                            modifiedValue[k] = modifiedValue[k - 1];
                        }
                        // Insert new value
                        bufferIndex[j] = modifiedIndex[i];
                        bufferValue[j] = modifiedValue[i];
                        break;
                    } else if (bufferIndex[j] == modifiedIndex[i]) {
                        bufferValue[j] = bufferValue[j] < modifiedValue[i] ? bufferValue[j] : modifiedValue[i];
                    }
                }
            }
        }
        for (int i = 0; i < totalNodes; i++) {
            if (bufferIndex[i] == -1) {
                modifiedIndex[i] = -1;
                modifiedValue[i] = -1;
                break;
            }
            printf("%d, ", bufferIndex[i]);
            modifiedIndex[i] = bufferIndex[i];
            modifiedValue[i] = bufferValue[i];
        }
        printf("\n");
    } else {
        int myArch = countBit(rank);
        for (int i = 0; i < myArch; i++) {
            MPI_Recv(modifiedIndex, totalNodes, datatype, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = 0; i < totalNodes; i++) {
                if (modifiedIndex[i] == -1) {
                    break;
                }
                for (int j = 0; j < totalNodes; j++) {
                    if (bufferIndex[j] > modifiedIndex[i]) {
                        // Move to the right
                        for (int k = totalNodes - 1; k >= j; k--) {
                            modifiedIndex[k] = modifiedIndex[k - 1];
                            modifiedValue[k] = modifiedValue[k - 1];
                        }
                        // Insert new value
                        bufferIndex[j] = modifiedIndex[i];
                        bufferValue[j] = modifiedValue[i];
                        length += 1;
                        break;
                    } else if (bufferIndex[j] == modifiedIndex[i]) {
                        bufferValue[j] = bufferValue[j] < modifiedValue[i] ? bufferValue[j] : modifiedValue[i];
                    }
                }
            }
        }

        if (myArch != 0) {
            for (int i = 0; i < totalNodes; i++) {
                if (bufferIndex[i] == -1) {
                    modifiedIndex[i] = -1;
                    modifiedValue[i] = -1;
                    break;
                }
                modifiedIndex[i] = bufferIndex[i];
                modifiedValue[i] = bufferValue[i];
            }
        }

        int shift = countBit(rank) + 1;
        int targetRank = (rank >> shift) << shift;  // Remove rightmost 1
        MPI_Send(modifiedIndex, length, datatype, targetRank, 0, MPI_COMM_WORLD);
    }
    free(bufferIndex);
    free(bufferValue);
}
