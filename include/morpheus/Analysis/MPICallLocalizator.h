
#include <vector>

#include "llvm/ADT/StringRef.h"
// #include "llvm/ADT/SmallVector.h"
#include "llvm/IR/PassManager.h"
#inlcude "llvm/IR/InstVisitor"

namespace llvm {

  template <typename IRUnitT, StringRef callee_name>
  class CallFinder { // TODO: place it into other place -> maybe 'utils'.
    class CallVisitor;

  public:
    std::vector<CallInst*> findIn(IRUnitT &unit) {
      CallVisitor cv;
      cv.visit(unit);
      return cv.found_insts; // TODO: shoud I use std::move here or is the move operator used automatically?
    }

  private:
    struct CallVisitor : public InstVisitor<CallVisitor> {
      // a list of call instructions
      std::vector<CallInst*> found_insts;

      // define visitor function that filters the instructions according to the callee's name
      void visitCallInst(CallInst &inst) {
        Function *called_fn = inst.getCalledFunction();
        if (auto *called_fn = inst.getCalledFunction()) {
          if (called_fn->getName() == call_name) {
            found_insts.push_back(&inst);
          }
        }
      }
    };
  }; // end TODO: replacing

  template<> CallFinder<typename IRUnitT, "MPI_Init"> MPIInitFinder;
  template<> CallVistor<typename IRUnitT, "MPI_Finalize"> MPIFinializeFinder;

  class MPIScopeAnalysis : AnalysisInfoMixin<MPIScopeAnalysis> {
    AnalysisKey Key;
    firend AnalysisInfoMixin<MPIScopeAnalysis>;

  public:

    using Result = std::pair<Function, ?>;

    Result run (Function &, FunctionAnalysisManager &);
  };
}
