#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <mpi.h>
#include <utility>
#include <vector>

#include "algorithms.h"

int Circulant(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
              int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  if (size == 1) {
    return MPI_SUCCESS;
  }

  const int *V = static_cast<const int *>(sendbuf);

  int max_size = sendcount * size;

  int *local = (int *)malloc(sendcount * sizeof(int));
  std::memcpy(local, V, sendcount * sizeof(int));
  int *outPtr = static_cast<int *>(recvbuf);
  int *recv_block = (int *)malloc(max_size * sizeof(int));
  int *inPtr = (int *)malloc(max_size * sizeof(int));

  int q = static_cast<int>(std::ceil(std::log2(size)));
  int *W;
  int *merged;

  // Determine pointers such that after the final round, the result gets already
  // merged into the recvbuf
  if (q % 2 != 0) {
    W = outPtr;
    merged = inPtr;
  } else {
    W = inPtr;
    merged = outPtr;
  }

  std::vector<int> skips(q + 1);
  skips[q] = size;
  for (int k = q - 1; k >= 0; --k) {
    skips[k] = (skips[k + 1] + 1) / 2;
  }

  for (int k = 0; k < q; ++k) {
    int sk = skips[k];
    int sk1 = skips[k + 1];
    int eps = sk1 & 0x1;
    int current_size = (sk - eps) * sendcount;
    int to = (rank - sk + eps + size) % size;
    int from = (rank + sk - eps) % size;

    if (eps == 1) {
      MPI_Sendrecv(W, current_size, sendtype, to, 0, recv_block, current_size, recvtype, from, 0,
                   comm, MPI_STATUS_IGNORE);

      mergeInts(W, current_size, recv_block, current_size, merged);

    } else {
      if (k == 0) {
        MPI_Sendrecv(V, current_size, sendtype, to, 0, recv_block, current_size, recvtype, from, 0,
                     comm, MPI_STATUS_IGNORE);

        std::memcpy(merged, recv_block, current_size * sizeof(int));

      } else {
        mergeInts(W, current_size - sendcount, local, sendcount, merged);

        MPI_Sendrecv(merged, current_size, sendtype, to, 0, recv_block, current_size, recvtype,
                     from, 0, comm, MPI_STATUS_IGNORE);

        mergeInts(W, current_size - sendcount, recv_block, current_size, merged);
      }
    }

    // Swap pointers
    int *temp = W;
    W = merged;
    merged = temp;
  }

  // Final merge with initial data
  mergeInts(W, max_size - sendcount, local, sendcount, merged);

  free(local);
  free(recv_block);
  free(inPtr);

  return MPI_SUCCESS;
}
