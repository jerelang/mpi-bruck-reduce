#include <mpi.h>
#include <algorithm>
#include <vector>
#include <cstring>

#include "algorithms.h"

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
    std::vector<tuwtype_t> temp(totalCount);
    tuwtype_t* inPtr  = data;
    tuwtype_t* outPtr = temp.data();

    int currBlockSize = recvcount;

    // Pairwise merges as a tree strcuture
    for (int step = 1; step < size; step <<= 1) {
        for (int blockIndex = 0; blockIndex < size; blockIndex += 2 * step) {
            int leftStartBlock  = blockIndex;
            int rightStartBlock = blockIndex + step;
            if (rightStartBlock >= size) {
                break; // no right block
            }

            int leftStartIdx  = leftStartBlock  * currBlockSize;
            int rightStartIdx = rightStartBlock * currBlockSize;
            int leftSize  = currBlockSize;
            int rightSize = currBlockSize;
            if (leftStartIdx + leftSize > totalCount) {
                leftSize = totalCount - leftStartIdx; // cut off near end
            }
            if (rightStartIdx + rightSize > totalCount) {
                rightSize = totalCount - rightStartIdx; // cut off near end
            }

            std::merge(inPtr + leftStartIdx, inPtr + (leftStartIdx + leftSize),
                       inPtr + rightStartIdx, inPtr + (rightStartIdx + rightSize),
                       outPtr + leftStartIdx);
        }

        int leftover = ((size / (2 * step)) * (2 * step)) * currBlockSize;
        if (leftover < totalCount) {
            std::memcpy(outPtr + leftover, inPtr + leftover,
                        (totalCount - leftover) * sizeof(tuwtype_t));
        }

        currBlockSize <<= 1;
        if (currBlockSize > totalCount) {
            currBlockSize = totalCount;
        }

        tuwtype_t* tmp = inPtr;
        inPtr = outPtr;
        outPtr = tmp;
    }

    std::memcpy(data, inPtr, totalCount * sizeof(tuwtype_t));
    return MPI_SUCCESS;
}