
//===----------------------------------------------------------------------===//
//
// Description of the file ... basic idea to provide a scope of elements
// that are involved in MPI either directly or indirectly
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/InstVisitor.h"

#include <vector>
#include <string>

namespace llvm {

  template <typename IRUnitT>
  class CallFinder { // >>> TODO: place it into other place -> maybe 'utils'; (?) private or public API?

    struct CallVisitor;

  public:
    std::vector<CallInst*> find_in(const std::string &callee_name, IRUnitT &unit) {
      CallVisitor cv(callee_name);
      cv.visit(unit);
      return cv.found_insts;
    }

  private:
    std::string callee_name;

    struct CallVisitor : public InstVisitor<CallVisitor> {

      std::string callee_name;

      CallVisitor(const std::string &callee_name) : callee_name(callee_name) { }
      // a list of call instructions
      std::vector<CallInst*> found_insts;

      // define visitor function that filters the instructions according to the callee's name
      void visitCallInst(CallInst &inst) {
        Function *called_fn = inst.getCalledFunction();
        if (auto *called_fn = inst.getCalledFunction()) {
          if (called_fn->getName() == callee_name) {
            found_insts.push_back(&inst);
          }
        }
      }
    };
  }; // <<< end TODO


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
