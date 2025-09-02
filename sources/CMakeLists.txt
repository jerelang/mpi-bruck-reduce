cmake_minimum_required(VERSION 3.10)
project(DisseminationReduce VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(MPI REQUIRED)

# Source files
add_executable(dissemination_reduce
    main.cpp
    algorithm0.cpp
    algorithm1.cpp
    algorithm2.cpp
    merge.cpp
)

# Include MPI headers
target_include_directories(dissemination_reduce PRIVATE ${MPI_CXX_INCLUDE_DIRS})

# Link MPI and optionally math
target_link_libraries(dissemination_reduce PRIVATE MPI::MPI_CXX m)

# Enable optimization and warnings
target_compile_options(dissemination_reduce PRIVATE -O3 -march=native -Wall -Wextra -pedantic)

enable_testing()
include(CTest)

add_test(NAME alg_baseline  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
         ${MPIEXEC_PREFLAGS} $<TARGET_FILE:dissemination_reduce> 128 0 --check --warmup 0 --repeat 1)
add_test(NAME alg_bruck     COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
         ${MPIEXEC_PREFLAGS} $<TARGET_FILE:dissemination_reduce> 128 1 --check --warmup 0 --repeat 1)
add_test(NAME alg_circulant COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
         ${MPIEXEC_PREFLAGS} $<TARGET_FILE:dissemination_reduce> 128 2 --check --warmup 0 --repeat 1)

set_tests_properties(alg_baseline alg_bruck alg_circulant
  PROPERTIES ENVIRONMENT "OMPI_MCA_rmaps_base_oversubscribe=1")