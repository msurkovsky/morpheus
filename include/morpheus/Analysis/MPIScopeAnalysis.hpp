
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

  struct MPIScopeResult {
    Function *scope; // it has to be a function ("worst case" is main)
    CallInst *start;
    CallInst *end;
  };

  class MPIScopeAnalysis : public AnalysisInfoMixin<MPIScopeAnalysis> { // both -*-module-*- and function analysis
    static AnalysisKey Key;
    friend AnalysisInfoMixin<MPIScopeAnalysis>;

  public:

    using Result = MPIScopeResult;

    MPIScopeResult run (Module &, ModuleAnalysisManager &);
  };
}
