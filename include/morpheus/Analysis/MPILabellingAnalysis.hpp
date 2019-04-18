
//===----------------------------------------------------------------------===//
//
// MPILabelingAnalysis
//
//===----------------------------------------------------------------------===//

#ifndef MRPH_MPI_LABELLING_H
#define MRPH_MPI_LABELLING_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/CallGraph.h"

#include "llvm/Support/raw_ostream.h"

#include <vector>
#include <queue>

namespace {
  enum struct MPICallType {
    DIRECT,
    INDIRECT,
  };
}


namespace llvm {

  class MPILabelling {
  public:
    using MPICheckpoints = std::queue<std::pair<BasicBlock::iterator const, MPICallType>>;

  private:
    enum ExplorationState {
      PROCESSING = 0,
      SEQUENTIAL,
      MPI_CALL,
      MPI_INVOLVED,
      MPI_INVOLVED_MEDIATELY,
    };

    using FunctionLabels = DenseMap<Function const *, ExplorationState>;
    using MPICalls = DenseMap<StringRef, std::vector<CallSite>>;
    using MPICheckpointsInBB = DenseMap<BasicBlock const *, MPICheckpoints>;

    FunctionLabels fn_labels;
    MPICalls mpi_calls;
    MPICheckpointsInBB bb_mpi_checkpoints;

    std::shared_ptr<CallGraph> cg;

  public:

    explicit MPILabelling(std::shared_ptr<CallGraph> cg);
    MPILabelling(const MPILabelling &labelling) = default;
    MPILabelling(MPILabelling &&labelling) = default;

    Instruction *get_unique_call(StringRef name) const;
    bool is_sequential(Function const *f) const;
    bool is_mpi_involved(Function const *f) const;
    // bool does_invoke_call(Function const *f, StringRef name) const; // TODO: move into scope analysis

    MPICheckpoints get_mpi_checkpoints(BasicBlock const *bb) const;

  private:

    ExplorationState explore_cgnode(CallGraphNode const *cgn);
    void save_checkpoint(Instruction *instr, MPICallType call_type);

    template<ExplorationState STATE> bool check_status(Function const *f) const {
      auto search = fn_labels.find(f);
      if (search == fn_labels.end()) {
        return false;
      }
      return search->second == STATE;
    }
  }; // MPILabelling


  class MPILabellingAnalysis : public AnalysisInfoMixin<MPILabellingAnalysis> {
    static AnalysisKey Key;
    friend AnalysisInfoMixin<MPILabellingAnalysis>;

  public:

    using Result = MPILabelling;

    Result run (Module &m, ModuleAnalysisManager &mam);

  }; // MPILabellingAnalysis
} // llvm

#endif // MRPH_MPI_LABELLING_H
