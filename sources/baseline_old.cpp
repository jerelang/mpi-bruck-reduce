#include <mpi.h>
#include <vector>
#include <algorithm>
#include <cstring>

#include "algorithms.h"


// The p-way merge is done using min-heaps with the following struct as nodes:
struct BlockElement {
  tuwtype_t value;
  int block_id;
  int index_in_block;
};

// Comparator for a min-heap
struct CompareBlockElement {
  bool operator()(const BlockElement &a, const BlockElement &b) const {
    return a.value > b.value;
  }
};

int HPC_AllgatherMergeBase(const void *sendbuf, int sendcount,
  MPI_Datatype sendtype, void *recvbuf, int recvcount,
  MPI_Datatype recvtype, MPI_Comm comm)
{
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);


  if (size == 1) {
    return MPI_SUCCESS;
  }

  MPI_Allgather(sendbuf, sendcount, sendtype,
                  recvbuf, recvcount, recvtype, comm);
 
  tuwtype_t *data = static_cast<tuwtype_t *>(recvbuf);
  int totalCount = size * recvcount;

  std::vector<tuwtype_t> merged(totalCount);

  std::vector<BlockElement> heap;
  heap.reserve(size);
  for (int block_id = 0; block_id < size; block_id++) {
    int start_idx = block_id * recvcount;
    if (recvcount > 0) {
      heap.push_back(BlockElement{ data[start_idx], block_id, 0 });
    }
  }
  // Default is a max-heap
  std::make_heap(heap.begin(), heap.end(), CompareBlockElement());

  for (int i = 0; i < totalCount; i++) {
    // swaps first with last element and rebuilds smaller heap
    std::pop_heap(heap.begin(), heap.end(), CompareBlockElement());
    BlockElement top = heap.back();
    // necessary to completely remove now last element
    heap.pop_back();

    merged[i] = top.value;

    if (top.index_in_block + 1 < recvcount) {
      int next_index = top.index_in_block + 1;
      int block_start = top.block_id * recvcount;
      heap.push_back(BlockElement{ data[block_start + next_index], top.block_id, next_index });
      std::push_heap(heap.begin(), heap.end(), CompareBlockElement());
    }
  }

  std::memcpy(data, merged.data(), totalCount * sizeof(tuwtype_t));

  return MPI_SUCCESS;
}