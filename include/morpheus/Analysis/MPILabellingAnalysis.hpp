
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
#include "llvm/Analysis/CallGraph.h"

#include "llvm/Support/raw_ostream.h"

#include <vector>

namespace {
  enum struct MPICallType {
    DIRECT,
    INDIRECT,
  };
}


namespace llvm {

  class MPILabellingAnalysis;

  class MPILabelling {

  private:
    enum ExplorationState {
      PROCESSING = 0,
      SEQUENTIAL,
      MPI_CALL,
      MPI_INVOLVED,
      MPI_INVOLVED_MEDIATELY,
    };

    // using CallTrack = std::forward_list<Instruction *>;

    using FunctionLabels = DenseMap<Function const *, ExplorationState>;
    using MPICalls = DenseMap<StringRef, std::vector<CallSite>>;

    // using  = std::pair<CallSite, MPICallType>; // TODO: come up with a name for this type

    // Storage for MPI affected calls withing basic block

    // TODO: maybe store the list of iterator points ... than I can go until "checkpoint"
    using BBCalls = DenseMap<BasicBlock const *, std::vector<std::pair<CallSite, MPICallType>>>;

    FunctionLabels fn_labels;
    MPICalls mpi_calls;

    BBCalls mpi_affected_bblocks;

    Function &root_fn;
    std::unique_ptr<CallGraph> cg;

  public:

    explicit MPILabelling(Function &f);
    MPILabelling(const MPILabelling &labelling);
    MPILabelling(MPILabelling &&labelling); // TODO: maybe use a default implementation

    // TODO: review
    Instruction *get_unique_call(StringRef name) const;
    bool is_sequential(Function const *f) const;
    bool is_mpi_involved(Function const *f) const;
    bool does_invoke_call(Function const *f, StringRef name) const;

    std::vector<CallInst *> get_indirect_mpi_calls(Function const *f) const;

  private:

    ExplorationState explore_function(Function const *f);

    ExplorationState explore_bb(BasicBlock const *bb);

    void explore_inst(Instruction const *inst,
                      Instruction const *caller=nullptr);

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

    Result run (Function &, FunctionAnalysisManager &);

  }; // MPILabellingAnalysis
} // llvm

#endif // MR_MPI_LABELLING_H
