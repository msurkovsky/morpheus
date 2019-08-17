
//===----------------------------------------------------------------------===//
//
// MPIScopeAnalysis
//
// Description of the file ... basic idea to provide a scope of elements
// that are involved in MPI either directly or indirectly
//
//===----------------------------------------------------------------------===//

#ifndef MRPH_MPI_SCOPE_H
#define MRPH_MPI_SCOPE_H

#include "morpheus/ADT/ParentPointerNode.hpp"
#include "morpheus/Analysis/MPILabellingAnalysis.hpp"

#include "llvm/ADT/ilist_iterator.h"
#include "llvm/ADT/simple_ilist.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/ModuleSummaryIndex.h"

#include <optional>
#include <unordered_map>
#include <iterator>

namespace llvm {

  class MPIScope {
    using VisitedNodes = std::unordered_map<CallGraphNode const *, bool>;

  public:
    using CallNodeDataT = std::pair<std::optional<Instruction *>, Function *>;
    using CallNode = PPNode<CallNodeDataT>;
    using CallsTrack = std::shared_ptr<CallNode>;

    ~MPIScope() = default;

    explicit MPIScope(ModuleSummaryIndex &index, MPILabelling &labelling, CallGraph &cg);
    MPIScope(const MPIScope &scope) = delete;
    MPIScope(MPIScope &&scope) = default;

    bool isValid();
    Function *getFunction();
    LoopInfo *getLoopInfo();

  private:
    void process_cgnode(CallGraphNode const *cgn, const CallsTrack &track, VisitedNodes &visited);

    Function *scope_fn;
    LoopInfo loop_info;
    std::unordered_map<Instruction const *, CallsTrack> instruction_calls_track;

    friend raw_ostream &operator<< (raw_ostream &out, const CallNodeDataT &data);
  }; // MPIScope


  class MPIScopeAnalysis : public AnalysisInfoMixin<MPIScopeAnalysis> {
    static AnalysisKey Key;
    friend AnalysisInfoMixin<MPIScopeAnalysis>;

  public:
    using Result = MPIScope;

    MPIScope run (Module &, ModuleAnalysisManager &);
  }; // MPIScopeAnalysis

} // llvm

#endif // MRPH_MPI_SCOPE_H
