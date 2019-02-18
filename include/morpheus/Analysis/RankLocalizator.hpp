
#include "llvm/IR/PassManager.h"

namespace llvm {

  class RankInfo;

  class RankAnalysis : public AnalysisInfoMixin<RankAnalysis> {
    friend AnalysisInfoMixin<RankAnalysis>;
    static AnalysisKey Key;

  public:
    using Result = RankInfo;

    RankInfo run(Module &, ModuleAnalysisManager &);
  };


  class RankInfo {

    // list of variables keeping rank value -> I should distinguish between alias and independent ranks
  };
}
