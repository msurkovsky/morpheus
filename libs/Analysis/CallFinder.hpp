
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/InstVisitor.h"
#include <vector>

using namespace llvm;

template <typename IRUnitT>
class CallFinder {

public:
  CallFinder() = delete;

  static std::vector<CallInst*> find_in(StringRef call_name, IRUnitT &unit) {
    CallVisitor cv(call_name);
    cv.visit(unit);
    return cv.found_insts;
  }

private:
  struct CallVisitor : public InstVisitor<CallVisitor> {

    // a name of call we are searching for
    StringRef callee_name;

    // a list of call instructions
    std::vector<CallInst*> found_insts;

    CallVisitor(StringRef callee_name) : callee_name(callee_name) { }

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
};
