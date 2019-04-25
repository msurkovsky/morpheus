
#include "mpi.h"

// std::string print_rank(int rank);

void ff () {
  MPI_Finalize();
}

void sink() {
  // do something
}

void invisible() { // the function ignored by analysis; not root and not reachable from any other
  invisible();
}

void do_sth() {
  // do something
  do_sth();
}

void unreachable_fn() {
  // do something
  sink();
}

int main (int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  int rank, size;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // std::cout << "I'm the '" << print_rank(rank) << "' process out of " << size << std::endl;
  if (rank == 0) {
    do_sth();
  }

  ff();
  return 0;
}

/*
std::string print_rank(int rank) {
  assert (rank >= 0);

  std::string sufix;
  sufix.reserve(2);

  switch (rank) {
  case 1:
    sufix = std::move("st");
    break;
  case 2:
    sufix = std::move("nd");
    break;
  case 3:
    sufix = std::move("rd");
    break;
  default:
    sufix = std::move("th");
  }

  return std::to_string(rank) + sufix;
}
*/
