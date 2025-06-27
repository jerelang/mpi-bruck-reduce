#include <mpi.h>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cmath>
#include <utility>
#include <iostream>

#include "algorithms.h"

int Dissemination(const void *sendbuf,
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

    // Compute how many blocks and in which rounds have to be sent additionaly to the partial merged lists
    std::vector<std::pair<int, int>> round_block_pairs;
    int l = size - (1 << static_cast<int>(std::log2(size)));
    int R = l;
    #ifdef DEBUG
        int rank_to_inspect = 1;
        if (rank == rank_to_inspect) {
            std::cout << "[Rank " << rank << "] l = " << l << "\n";
        }
    #endif
    while (R > 0) {
        int n = static_cast<int>(std::log2(R));
        int r = R - (1 << n); 
        round_block_pairs.emplace_back(n, r);
        
        #ifdef DEBUG
            if (rank == rank_to_inspect) {
                std::cout << "[Rank " << rank << "] Adding round pair (n = " << n << ", r = " << r << "), R = " << R << "\n";
                std:: cout << "n: " << n << "\n";
            }
        #endif
    
        R = r;
    }
    std::reverse(round_block_pairs.begin(), round_block_pairs.end());

    #ifdef DEBUG
        if (rank == rank_to_inspect) {
            std::cout << "[Rank " << rank << "] Final round_block_pairs:\n";
            for (const auto& p : round_block_pairs) {
                std::cout << "  (round = " << p.first << ", blocks = " << p.second << ")\n";
            }
        }
    #endif

    const int *send_data = static_cast<const int *>(sendbuf);

    int max_size = sendcount * size;
    int current_size = sendcount;

    // Preallocate memory for arrays
    int* inPtr = (int*)malloc(max_size * sizeof(int));
    int* recv_block = (int*)malloc(max_size * sizeof(int));
    int* outPtr = static_cast< int *>(recvbuf);
    int* partial = (int*)malloc((max_size / 2 + 1) * sizeof(int));

    int log_p = static_cast<int>(std::log2(size));

    int* local;
	int* merged;

	// determine pointers such that after the final round, the result gets already merged into the recvbuf
	if (static_cast<int>(std::ceil(std::log2(size))) % 2 == 0) {
        local  = outPtr;
        merged = inPtr;
    } else {
        local  = inPtr;
        merged = outPtr;
    }
    memcpy(local, send_data, sendcount * sizeof(int));

    int r = -1; // normal round if r = -1
    int idx = 0;
    int k;
    int original_size;
    int partial_size = 0;
    for (k = 0; k < log_p; ++k) {
        int s_k = 1 << k;
        int partner_send = (rank - s_k + size) % size;
        int partner_recv = (rank + s_k) % size;

        // Check if additional blocks have to be sent unmerged this round
        if (idx < static_cast<int>(round_block_pairs.size()) && round_block_pairs[idx].first == k) {
            r = round_block_pairs[idx].second;
            idx++;
        }

        original_size = current_size;

        // If r=0, only the local merged list will be saved in the partial buffer
        // If r>0, additional blocks are sent and received, which have to be merged with the local list and then stored in the partial buffer
        if (r > 0 && partial_size > 0){
            memcpy(local + original_size, partial, partial_size * sizeof(int));
            current_size += partial_size;
        }

        #ifdef DEBUG
            if (rank == rank_to_inspect) {
                std::cout << "[Round " << k << "]\n";
                std::cout << "  r = " << r << ", s_k = " << s_k << ", partner_send = " << partner_send << ", partner_recv = " << partner_recv << "\n";
                std::cout << "  original_size = " << original_size << ", current_size = " << current_size << "\n";
                std::cout << "  partial.size() = " << partial_size << "\n";
                std::cout << "  partial contents: ";
                for (int i = 0; i < static_cast<int>(partial_size); ++i)
                    std::cout << partial[i] << " ";
                std::cout << "\n";
            }
        #endif

        MPI_Sendrecv(local, current_size, sendtype, partner_send, 0,
                     recv_block, current_size, recvtype, partner_recv, 0,
                     comm, MPI_STATUS_IGNORE);

        mergeInts(local, original_size, recv_block, original_size, merged);

        if (r > 0) {
            mergeInts(local, original_size, recv_block + original_size, partial_size, partial);
            partial_size += original_size;
        } else if (r == 0){
            partial_size += original_size;
            memcpy(partial, local, original_size * sizeof(int));
        }

        #ifdef DEBUG
            if (rank == rank_to_inspect){
                std::cout << "  partial contents after copy: ";
                for (int i = 0; i < static_cast<int>(partial_size); ++i)
                    std::cout << partial[i] << " ";
                std::cout << "\n";
                std::cout << "  partial_size = " << partial_size << "\n";
                std::cout << "  local contents: ";
                for (int i = 0; i < static_cast<int>(current_size); ++i)
                    std::cout << local[i] << " ";
                std::cout << "\n";
                std::cout << "  merged contents: ";
                for (int i = 0; i < static_cast<int>(max_size); ++i)
                    std::cout << merged[i] << " ";
                std::cout << "\n";
            }
        #endif  

        // Swap pointers
        int* temp = local;
        local = merged;
        merged = temp;

        current_size = 2 * original_size;

        if (current_size > max_size)
            current_size = max_size;
        r = -1; // reset block number
    }

    // additional last round if p is not a power of 2
    if (l != 0){
        int s_k = 1 << k;
        int partner_send = (rank - s_k + size) % size;
        int partner_recv = (rank + s_k) % size;

        #ifdef DEBUG
            if (rank == rank_to_inspect) {
                std::cout << "[Final Round]\n";
                std::cout << "  last_size = " << partial_size << "\n";
                std::cout << "  partial contents: ";
                for (int i = 0; i < partial_size; ++i)
                    std::cout << partial[i] << " ";
                std::cout << "\n";
            }
        #endif

        MPI_Sendrecv(partial, partial_size, sendtype, partner_send, 0,
                     recv_block, partial_size, recvtype, partner_recv, 0,
                     comm, MPI_STATUS_IGNORE);

        mergeInts(local, current_size, recv_block, partial_size, merged);

        #ifdef DEBUG
            if (rank == rank_to_inspect){
                std::cout << "  merged contents: ";
                for (int i = 0; i < static_cast<int>(max_size); ++i)
                    std::cout << merged[i] << " ";
                std::cout << "\n";
            }
        #endif
        // Swap pointers for the final merge
        int* temp = local;
        local = merged;
        merged = temp;
    }

    free(inPtr);
    free(recv_block);
    free(partial);

    return MPI_SUCCESS;
}
