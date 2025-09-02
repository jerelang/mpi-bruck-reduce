#include <algorithm>
#include <cstring>
#include <iostream>
#include <mpi.h>
#include <vector>

#include "algorithms.h"

int Baseline(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
             int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  if (size == 1) {
    return MPI_SUCCESS;
  }

  MPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);

  int *data = static_cast<int *>(recvbuf);
  int totalCount = size * recvcount;
  if (totalCount == 0) {
    return MPI_SUCCESS;
  }

  // Ping Pong buffer approach to avoid extra mcopy in each merge round
  int *temp = (int *)malloc(totalCount * sizeof(int));
  int *inPtr = data;
  int *outPtr = temp;

  int blockSize = recvcount;
  int numBlocks = size;

  while (numBlocks > 1) {
    int outBlock = 0;

    for (int i = 0; i + 1 < numBlocks; i += 2) {
      int leftStart = i * blockSize;
      int rightStart = (i + 1) * blockSize;

      int leftSize = std::min(blockSize, totalCount - leftStart);
      int rightSize = std::min(blockSize, totalCount - rightStart);

      mergeInts(inPtr + leftStart, leftSize, inPtr + rightStart, rightSize, outPtr + leftStart);

      ++outBlock;
    }

    // If we have an odd block left, copy it directly
    if (numBlocks % 2 == 1) {
      int lastStart = (numBlocks - 1) * blockSize;
      int lastSize = std::min(blockSize, totalCount - lastStart);
      std::memcpy(outPtr + outBlock * (2 * blockSize), inPtr + lastStart, lastSize * sizeof(int));
      ++outBlock;
    }

    blockSize *= 2;
    numBlocks = outBlock;

    // Swap pointers
    int *inPtr_temp = inPtr;
    inPtr = outPtr;
    outPtr = inPtr_temp;
  }

  if (inPtr != data) {
    std::memcpy(data, inPtr, totalCount * sizeof(int));
  }

  free(temp);
  return MPI_SUCCESS;
}