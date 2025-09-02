#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include "algorithms.h"

void print_usage(int rank, const char* prog) {
    if (rank == 0) {
        std::cout << "Usage: " << prog << " [msg_size] [algorithm] [--check] "
                  << "[--warmup <int>] [--repeat <int>]\n"
                  << "  msg_size   : Number of elements per process (default: 1000)\n"
                  << "  algorithm  : 0=Baseline, 1=Bruck, 2=Circulant (default: 0)\n"
                  << "  --check    : Enable correctness verification\n"
                  << "  --warmup   : Number of warm-up iterations (default: 2)\n"
                  << "  --repeat   : Number of timed repetitions (default: 10)\n";
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0) {
            std::cerr << "At least 2 processes are necessary.\n";
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    int msg_size = 1000;
    int algorithm = 0;
    int warmup = 2;
    int repeat = 10;
    bool check = false;

    // Parse args
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--check") == 0) {
            check = true;
        } else if (std::strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
            warmup = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--repeat") == 0 && i + 1 < argc) {
            repeat = std::atoi(argv[++i]);
        } else if (msg_size == 1000) {
            msg_size = std::atoi(argv[i]);
        } else if (algorithm == 0) {
            algorithm = std::atoi(argv[i]);
        } else {
            print_usage(rank, argv[0]);
            MPI_Finalize();
            return EXIT_FAILURE;
        }
    }

    if (algorithm < 0 || algorithm > 2 || msg_size < 1 || warmup < 0 || repeat < 1) {
        print_usage(rank, argv[0]);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    std::vector<int> sendbuf(msg_size);
    std::vector<int> recvbuf(msg_size * size);
    std::vector<double> runtimes(repeat);

    // Fill input with rank-dependent values
    for (int i = 0; i < msg_size; ++i){
        sendbuf[i] = rank * size + i;
        recvbuf[i + (rank * msg_size)] = rank * size + i; // Also fill recvbuf for MPI_IN_PLACE usage
    }
    for (int r = 0; r < warmup + repeat; ++r) {
        MPI_Barrier(MPI_COMM_WORLD);
        double start = MPI_Wtime();

        switch (algorithm) {
            case 0:
                Baseline(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                       recvbuf.data(), msg_size, MPI_INT, MPI_COMM_WORLD);
                break;
            case 1:
                Dissemination(sendbuf.data(), msg_size, MPI_INT,
                                        recvbuf.data(), msg_size, MPI_INT, MPI_COMM_WORLD);
                break;
            case 2:
                Circulant(sendbuf.data(), msg_size, MPI_INT,
                                            recvbuf.data(), msg_size, MPI_INT, MPI_COMM_WORLD);
                break;
        }

        MPI_Barrier(MPI_COMM_WORLD);
        double end = MPI_Wtime();

        if (r >= warmup)
            runtimes[r - warmup] = end - start;
    }

    // Average runtime over all processes
    double local_avg = 0.0;
    for (double t : runtimes) local_avg += t;
    local_avg /= repeat;

    double global_avg;
    MPI_Allreduce(&local_avg, &global_avg, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "Algorithm: " << algorithm
                  << ", Msg size: " << msg_size
                  << ", Avg time over " << repeat << " runs: "
                  << global_avg * 1e6 << " us\n";
    }

    // Correctness check
    if (check) {
        std::vector<int> reference(msg_size * size);

        int err = MPI_Allgather(sendbuf.data(), msg_size, MPI_INT,
                                reference.data(), msg_size, MPI_INT, MPI_COMM_WORLD);
        if (err != MPI_SUCCESS) {
            std::cerr << "MPI_Allgather failed with error code " << err << "\n";
            MPI_Abort(MPI_COMM_WORLD, err);
        }

        if (size > 1) {
            std::sort(reference.begin(), reference.end());
        }
        for (int i = 0; i < msg_size * size; ++i) {
            if (recvbuf[i] != reference[i]) {
                std::cerr << "Process " << rank << ": mismatch at index " << i
                          << " (got " << recvbuf[i] << ", expected " << reference[i] << ")\n";
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }

        if (rank == 0)
            std::cout << "Correctness check passed.\n";
    }

    MPI_Finalize();
    return 0;
}