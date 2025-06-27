#ifndef ALGORITHMS_H_
#define ALGORITHMS_H_

#include <mpi.h>

void mergeInts(const int* a, int aSize, const int* b, int bSize, int* out);

int Baseline(const void *sendbuf, int sendcount,
                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                           MPI_Datatype recvtype, MPI_Comm comm);

int Dissemination(const void *sendbuf, int sendcount,
                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                            MPI_Datatype recvtype, MPI_Comm comm);

int Circulant(const void *sendbuf, int sendcount,
                                MPI_Datatype sendtype, void *recvbuf,
                                int recvcount, MPI_Datatype recvtype,
                                MPI_Comm comm);

#ifdef __cplusplus

#endif
#endif // ALGORITHMS_H_
