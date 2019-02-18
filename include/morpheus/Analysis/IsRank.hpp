
#include "llvm/IR/PassManager.h"

namespace llvm {

  class IsRank : public AnalysisInfoMixin<IsRank> {
    friend AnalysisInfoMixin<IsRank>;
    static AnalysisKey Key;

  public:
    struct Result {
      bool rank;
    };

    Result run(Function &, FunctionAnalysisManager &);
  };
}
