#include <mpi.h>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cmath>
#include <utility>
#include <iostream>

#include "algorithms.h"

int HPC_AllgatherMergeBruck(const void *sendbuf,
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

    int rank_to_inspect = 1;

    // Compute how many blocks and in which rounds have to be sent additionaly to the partial merged lists
    std::vector<std::pair<int, int>> round_block_pairs;
    int l = size - (1 << static_cast<int>(std::log2(size)));
    int R = l;
    
    if (rank == rank_to_inspect) {
        std::cout << "[Rank " << rank << "] l = " << l << "\n";
    }
    
    while (R > 0) {
        int n = static_cast<int>(std::log2(l)); //HERE IS THE ERROR THIS SHIT DONT WORK, TOO HIGH
        int r = R - (1 << n); 
        round_block_pairs.emplace_back(n, r);
    
        if (rank == rank_to_inspect) {
            std::cout << "[Rank " << rank << "] Adding round pair (n = " << n << ", r = " << r << "), R = " << R << "\n";
            std:: cout << "n: " << n << "\n";
        }
    
        R = r;
    }
    
    std::reverse(round_block_pairs.begin(), round_block_pairs.end());
    
    if (rank == rank_to_inspect) {
        std::cout << "[Rank " << rank << "] Final round_block_pairs:\n";
        for (const auto& p : round_block_pairs) {
            std::cout << "  (round = " << p.first << ", blocks = " << p.second << ")\n";
        }
    }
    
    const tuwtype_t *send_data = static_cast<const tuwtype_t *>(sendbuf);
    tuwtype_t* output = static_cast<tuwtype_t*>(recvbuf);

    int max_size = sendcount * size;
    int current_size = sendcount;
    std::vector<tuwtype_t> local(send_data, send_data + sendcount);

    // Preallocate full size
    local.resize(max_size);
    std::vector<tuwtype_t> recv_block(max_size);
    std::vector<tuwtype_t> merged(max_size);
    // maximal size achieved when p is one off its next power of 2, e.g. p=63 -> max_size=63-32 -> p / 2
    std::vector<tuwtype_t> partial(static_cast<int>(max_size/2) + 1);

    int log_p = static_cast<int>(std::log2(size));
    int r = -1;
    int idx = 0;
    int k;
    int original_size;
    int partial_size = 0;
    for (k = 0; k < log_p; ++k) {
        int s_k = 1 << k;
        int partner_send = (rank - s_k + size) % size;
        int partner_recv = (rank + s_k) % size; 

        // Check if additional blocks have to be sent unmerged this round
        if (idx < round_block_pairs.size() && round_block_pairs[idx].first == k) {
            r = round_block_pairs[idx].second;
            idx++;
        }
        
        original_size = current_size;

        // If r=0, only the local merged list will be saved in the partial buffer
        // If r>0, additional blocks are sent and received, which have to be merged with the local list and then stored in the partial buffer
        if (r > 0 && !partial.empty()){
            partial_size += (r + 1) * sendcount;
            local.insert(local.end(), partial.begin(), partial.end());
            current_size += partial_size;
        }

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

        MPI_Sendrecv(local.data(), current_size, sendtype, partner_send, 0,
                    recv_block.data(), current_size, recvtype, partner_recv, 0,
                    comm, MPI_STATUS_IGNORE);

        mergeInts(local.data(), original_size, recv_block.data(), original_size, merged.data());

        if (r > 0) {
            mergeInts(local.data(), original_size, recv_block.data() + original_size, partial_size, partial.data());
            partial_size += original_size;
        } else if (r == 0){
            partial_size += original_size;
            std::copy(local.data(), local.data() + original_size, partial.data());
        }
        if (rank == rank_to_inspect){
            std::cout << "  patial contents after copy: ";
            for (int i = 0; i < static_cast<int>(partial.size()); ++i)
                std::cout << partial[i] << " ";
            std::cout << "\n";
            std::cout << "  partial.size() = " << partial_size << "\n";
        }
        if (rank == rank_to_inspect){
            std::cout << "  local contents: ";
            for (int i = 0; i < static_cast<int>(local.size()); ++i)
                std::cout << local[i] << " ";
            std::cout << "\n";
        }
        if (rank == rank_to_inspect){
            std::cout << "  merged contents: ";
            for (int i = 0; i < static_cast<int>(merged.size()); ++i)
                std::cout << merged[i] << " ";
            std::cout << "\n";
        }

        std::swap(local, merged);
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

        if (rank == rank_to_inspect) {
            std::cout << "[Final Round]\n";
            std::cout << "  last_size = " << partial_size << "\n";
            std::cout << "  partial contents: ";
            for (int i = 0; i < partial_size; ++i)
                std::cout << partial[i] << " ";
            std::cout << "\n";
        }
        MPI_Sendrecv(partial.data(), partial_size, sendtype, partner_send, 0,
        recv_block.data(), partial_size, recvtype, partner_recv, 0,
        comm, MPI_STATUS_IGNORE);
        mergeInts(local.data(), current_size, recv_block.data(), partial_size, merged.data());
        if (rank == rank_to_inspect){
            std::cout << "  merged contents: ";
            for (int i = 0; i < static_cast<int>(merged.size()); ++i)
                std::cout << merged[i] << " ";
            std::cout << "\n";
        }
        std::swap(local, merged);
    }
    std::copy(local.data(), local.data() + current_size + partial_size, output);
    return MPI_SUCCESS;
}
