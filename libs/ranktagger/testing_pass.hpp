
#include "llvm/IR/PassManager.h"

namespace llvm {
  struct TestingFnPass : public PassInfoMixin<TestingFnPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
  };
}
