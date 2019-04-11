
//===----------------------------------------------------------------------===//
//
// MPILabelingAnalysis (*)
//
//===----------------------------------------------------------------------===//

#ifndef MR_MPI_LABELLING_H
#define MR_MPI_LABELLING_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/PassManager.h"

#include "llvm/Support/raw_ostream.h"

#include <vector>


namespace llvm {

  class MPILabellingAnalysis;

  class LabellingResult {
    friend MPILabellingAnalysis;

    enum ExplorationState {
      PROCESSING = 0,
      SEQUENTIAL,
      MPI_CALL,
      MPI_INVOLVED,
      MPI_INVOLVED_MEDIATELY,
    };

    using FunctionLabels = DenseMap<Function const *, ExplorationState>;
    using MPICalls = DenseMap<StringRef, std::vector<CallInst *>>;
    using FunctionCalls = DenseMap<Function const *, std::vector<CallInst *>>;

    FunctionLabels fn_labels;
    MPICalls mpi_calls;
    FunctionCalls direct_mpi_calls;
    FunctionCalls mediate_mpi_calls;

    // TODO: do I need to store parentage relation? - NO (11.4.19)
  public:
    // some public methods

  protected:

    ExplorationState explore_function(const Function *f);

    ExplorationState explore_bb(const BasicBlock *bb,
                                std::vector<CallInst *> &direct_mpi_calls,
                                std::vector<CallInst *> &mediate_mpi_calls);

    void map_mpi_call(StringRef name, CallInst *inst);

  }; // LabellingResult

  class MPILabellingAnalysis : public AnalysisInfoMixin<MPILabellingAnalysis> {
    static AnalysisKey Key;
    friend AnalysisInfoMixin<MPILabellingAnalysis>;

  public:

    LabellingResult run (Function &, FunctionAnalysisManager &);

  }; // MPILabellingAnalysis
} // llvm

#endif // MR_MPI_LABELLING_H
