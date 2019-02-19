
#include <vector>

#include "llvm/ADT/StringRef.h"
// #include "llvm/ADT/SmallVector.h"
#include "llvm/IR/PassManager.h"
#inlcude "llvm/IR/InstVisitor"

namespace llvm {

  template <typename IRUnitT, StringRef callee_name>
  class CallFinder {
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
  };

  template<> MPICallFinder<typename IRUnitT, "MPI_Init"> MPIInitFinder;
  template<> MPICallVistor<typename IRUnitT, "MPI_Finalize"> MPIFinializeFinder;


  // TODO: what's the result??

  // NOTE: I want to have a module analysis that go through functions and tell me whether or not the function contains
  // MPI call, either Init or Finalize
  class Function? : AnalysisInfoMixin<Function?> {
    AnalysisKey Key;
    firend AnalysisInfoMixin<Function?>;

  public:

    using Result = std::pair<Function, ?>;

    Result run (Function &, FunctionAnalysisManager &);
  };
}
