
#include <vector>

#include "llvm/IR/PassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InstVisitor.h"

namespace llvm {

  struct CallGetRankVisitor : public InstVisitor<CallGetRankVisitor> {

    std::vector<CallInst*> mpi_rank_insts;

    void visitCallInst(CallInst &I) {
      Function *called_fn = I.getCalledFunction();
      if (called_fn) {
        if (called_fn->getName() == "MPI_Comm_rank") {
          mpi_rank_insts.push_back(&I);
        }
      }
    }
  };

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
