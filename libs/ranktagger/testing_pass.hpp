
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

  class TestingAnalysisPass : public AnalysisInfoMixin<TestingAnalysisPass> {

    friend AnalysisInfoMixin<TestingAnalysisPass>;
    static AnalysisKey Key;

  public:
    struct Result {
      int result = 7;
    };

    Result run(Function &F, FunctionAnalysisManager &FAM) {
      errs() << "Analysis run\n";
      return Result();
    }
  };


  struct TestingFnPass : public PassInfoMixin<TestingFnPass> {

  public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
  };
}
