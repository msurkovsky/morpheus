
//===----------------------------------------------------------------------===//
//
// Description of the file ... basic idea to provide a scope of elements
// that are involved in MPI either directly or indirectly
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/PassManager.h"

#include <vector>
#include <string>

namespace llvm {

  class MPIScopeAnalysis;

  struct MPIScopeResult { // TODO: make the data private
    Function *scope; // it has to be a function ("worst case" is main)
    CallInst *start; // Q?: This is a pointer on call that may be either
                     //     an MPI_Init call or another mediately calling MPI_Init.
                     //     The Question is how to cope with it? ... Or what to expect
    CallInst *end;

    CallInst *init;
    CallInst *finalize;

    // NOTE:
    // Well, in the end I need a kind of navigation trough the scope. Therefore, a set of queries needs to be defined.

    // TODO: define it as an iterator over "unfolded" scope
  };

  class MPIScopeAnalysis : public AnalysisInfoMixin<MPIScopeAnalysis> { // both -*-module-*- and function analysis
    static AnalysisKey Key;
    friend AnalysisInfoMixin<MPIScopeAnalysis>;

  public:

    using Result = MPIScopeResult;

    MPIScopeResult run (Module &, ModuleAnalysisManager &);
  };
}
