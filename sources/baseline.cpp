#include <mpi.h>
#include <algorithm>
#include <vector>
#include <cstring>
#include <iostream>

#include "algorithms.h"

void mergeInts(const int* a, int aSize, const int* b, int bSize, int* out) {
    int i = 0, j = 0, k = 0;
    while (i < aSize && j < bSize) {
        out[k++] = (a[i] < b[j]) ? a[i++] : b[j++];
    }
    while (i < aSize) out[k++] = a[i++];
    while (j < bSize) out[k++] = b[j++];
}

int HPC_AllgatherMergeBase(const void *sendbuf,
                           int sendcount,
                           MPI_Datatype sendtype,
                           void *recvbuf,
                           int recvcount,
                           MPI_Datatype recvtype,
                           MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    if (size == 1) {
        return MPI_SUCCESS;
    }

    MPI_Allgather(sendbuf, sendcount, sendtype,
                  recvbuf, recvcount, recvtype, comm);

    tuwtype_t* data = static_cast<tuwtype_t*>(recvbuf);
    int totalCount = size * recvcount;
    if (totalCount == 0) {
        return MPI_SUCCESS;
    }

    // Ping Pong buffer approach to avoid extra mcopy in each merge round
    tuwtype_t* temp = (tuwtype_t*)malloc(totalCount * sizeof(tuwtype_t));
    tuwtype_t* inPtr  = data;
    tuwtype_t* outPtr = temp;

    int blockSize = recvcount;
    int numBlocks = size;

    while (numBlocks > 1) {
        int outBlock = 0;

        for (int i = 0; i + 1 < numBlocks; i += 2) {
            int leftStart = i * blockSize;
            int rightStart = (i + 1) * blockSize;

            int leftSize = std::min(blockSize, totalCount - leftStart);
            int rightSize = std::min(blockSize, totalCount - rightStart);

            mergeInts(inPtr + leftStart, leftSize,
                    inPtr + rightStart, rightSize,
                    outPtr + leftStart);

            ++outBlock;
        }

        // If we have an odd block left, copy it directly
        if (numBlocks % 2 == 1) {
            int lastStart = (numBlocks - 1) * blockSize;
            int lastSize = std::min(blockSize, totalCount - lastStart);
            std::memcpy(outPtr + outBlock * (2*blockSize),
                        inPtr + lastStart,
                        lastSize * sizeof(tuwtype_t));
            ++outBlock;
        }

        blockSize *= 2;
        numBlocks = outBlock;

        // Swap pointers
		tuwtype_t* temp = inPtr;
		inPtr = outPtr;
		outPtr = temp;
    }

    if (inPtr != data) {
        std::memcpy(data, inPtr, totalCount * sizeof(tuwtype_t));
    }
    return MPI_SUCCESS;
}