#include "mpi.h"

#include <functional>

void do_nothing() {
  // do nothing
}

int get_rank() {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  return rank;
}

int main (int argc, char *argv[]) {
  enum { TAG_A };

  std::function<int()> GetRank = get_rank;
  MPI_Init(&argc, &argv);

  int rank = GetRank(), size;
  // MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (rank == 0) {
    int ns[size-1];
    for (int i=0, src=1; src < size; i++, src++) {
      MPI_Recv(&ns[i], 1, MPI_INT, src, TAG_A, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
  } else if (rank == 1) {
    do_nothing();
  }else {
    MPI_Send(&rank, 1, MPI_INT, 0, TAG_A, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}
