# Modified Bruck/Dissemination Algorithm for Commutative Reduction

This repository presents a modified variant of the Bruck/Dissemination algorithm for commutative reduction operations in MPI. The implementation performs a global merge of sorted local blocks across all processes, achieving the same asymptotic communication and computation complexity as standard MPI_Allgather-based approaches.

Although this implementation uses element-wise merging as the reduction operator, the structure of the algorithm supports any commutative operation.

Unlike circulant-style Allreduce variants, which require different communication partners based on the number of processes, this version preserves the fixed partner structure of Bruck’s algorithm—making it suitable for systems with pre-optimized communication schedules or strict communication policies. To support correct aggregation, the Bruck/Dissemination algorithm is modified to transmit additional data only in specific, predetermined rounds—precisely when required to avoid loss of information due to overlapping reductions. This extra data is accumulated in a separate buffer and sent in a final additional round. The selection of rounds and data is optimal: it represents the minimal necessary overhead, resulting in at most 1.5× the total data volume of the standard Bruck’s algorithm in the worst case.

For pseudo code and a detailed space and time complexity analysis of the three implemented algorithms, see [details (PDF)](./details.pdf).


## Implemented Algorithms

| ID | Name       | Description                                                                          |
|----|------------|--------------------------------------------------------------------------------------|
| 0  | Baseline   | Standard `MPI_Allgather` followed by local sort                                      |
| 1  | Bruck      | Dissemination based allgather and merge with extra data sends                        |
| 2  | Circulant  | Roughly-halving skips communication, optimal but different communication pairs       |

Each algorithm is implemented in its own source file and selected at runtime via command-line arguments.

## Build Instructions

### Requirements

- C++17-compatible compiler (e.g., `g++`, `clang++`)
- MPI library (e.g., OpenMPI or MPICH)
- CMake version 3.10 or higher

### Build Steps

```bash
git clone https://github.com/jerelang/mpi-bruck-reduce.git
cd mpi-bruck-reduce
cmake -B build
cmake --build build
```

This will generate the `dissemniation_reduce` executable in the `build/` directory.

## Usage

Run the program using `mpirun` or `mpiexec`:

```bash
mpirun -np <num_processes> ./build/dissemniation_reduce [msg_size] [algorithm] [--check] [--warmup <int>] [--repeat <int>]
```

### Arguments

- `msg_size` (default: `1000`)  
  Number of integers sent by each process
- `algorithm` (default: `0`)  
  `0` = Baseline  
  `1` = Bruck  
  `2` = Circulant

### Optional Flags

- `--check`  
  Enables correctness verification against a sorted `MPI_Allgather` reference
- `--warmup <int>`  
  Number of warm-up iterations before timing (default: `2`)
- `--repeat <int>`  
  Number of timed iterations to average (default: `10`)

### Example

```bash
mpirun -np 4 ./build/dissemniation_reduce 10000 1 --check --repeat 5
```

Runs the Bruck algorithm with 10,000 integers per process, performs correctness verification, and averages over 5 timed iterations.


## Code Structure

```
.
├── main.cpp              # Entry point and benchmark driver
├── baseline.cpp          # Baseline allgather + sort
├── algorithm1.cpp        # Bruck-style dissemination algorithm
├── algorithm2.cpp        # Circulant algorithm
├── merge.cpp             # Sorted merge routine
├── algorithms.h          # Common declarations
├── CMakeLists.txt        # Build configuration
```
