
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


namespace {
  enum struct ExplorationState {
    PROCESSING = 0,
    SEQUENTIAL,
    MPI_CALL,
    MPI_INVOLVED,
    MPI_INVOLVED_MEDIATELY,
  };
}


namespace llvm {

  class MPILabellingAnalysis;
  class LabellingResult {
    friend MPILabellingAnalysis;

    using FunctionLabels = DenseMap<const Function *, ExplorationState>;
    using MPICalls = DenseMap<StringRef, std::vector<CallInst *> >;

    FunctionLabels fn_labels;
    MPICalls mpi_calls;
    // TODO: do I need to store parentage relation?
  public:
    // some public methods

  protected:

    ExplorationState explore_function(const Function *f);

    ExplorationState explore_bb(const BasicBlock *bb);

    void map_mpi_call(StringRef name, CallInst *inst);

  }; // LabellingResult

  class MPILabellingAnalysis : public AnalysisInfoMixin<MPILabellingAnalysis> {
    static AnalysisKey Key;
    friend AnalysisInfoMixin<MPILabellingAnalysis>;

  public:

    LabellingResult run (Function &, FunctionAnalysisManager &);

  }; // MPILabellingAnalysis
} // llvm


namespace {

  using namespace llvm;

  raw_ostream & operator<< (raw_ostream &out, const ExplorationState &s) {
    out << "Exploration state =";
    switch(s) {
    case ExplorationState::PROCESSING:
      out << " PROCESSING";
      break;
    case ExplorationState::SEQUENTIAL:
      out << " SEQUENTIAL";
      break;
    case ExplorationState::MPI_CALL:
      out << " MPI_CALL";
      break;
    case ExplorationState::MPI_INVOLVED:
      out << " MPI_INVOLVED";
      break;
    case ExplorationState::MPI_INVOLVED_MEDIATELY:
      out << " MPI_INVOLVED_MEDIATELY";
      break;
    }
    out << "\n";
    return out;
  }
}

#endif // MR_MPI_LABELLING_H
