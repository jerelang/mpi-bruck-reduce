cmake_minimum_required(VERSION 3.10)
project(DisseminationReduce LANGUAGES CXX)

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