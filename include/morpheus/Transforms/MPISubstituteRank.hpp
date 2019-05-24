
//===----------------------------------------------------------------------===//
//
// MPIPruneProcess
//
//===----------------------------------------------------------------------===//

#ifndef MRPH_MPI_PRUNE_PROCESS_H
#define MRPH_MPI_PRUNE_PROCESS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

  struct MPISubstituteRankPass : public PassInfoMixin<MPISubstituteRankPass> {

    PreservedAnalyses run (Module &m, ModuleAnalysisManager& am);
  };
} // end llvm

#endif // MRPH_MPI_PRUNE_PROCESS_H
