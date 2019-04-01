
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/InstVisitor.h"
#include <vector>
#include <functional>

using namespace llvm;

template <typename IRUnitT>
class CallFinder {

public:
  CallFinder() = delete;

  static std::vector<CallInst*> find_in(
      IRUnitT &unit,
      std::function<bool(const CallInst&)> filter=[](const CallInst&) { return true; }) {

    CallVisitor cv(filter);
    cv.visit(unit);
    return cv.found_insts;
  }

private:
  struct CallVisitor : public InstVisitor<CallVisitor> {

    // a name of call we are searching for
    std::function<bool(const CallInst&)> filter;

    // a list of call instructions
    std::vector<CallInst*> found_insts; // TODO: is it safe to store the pointer here?

    CallVisitor(std::function<bool(const CallInst&)> filter) : filter(filter) { }

    // define visitor function that filters the instructions according to the callee's name
    void visitCallInst(CallInst &inst) {
      if (filter(inst)) {
        found_insts.push_back(&inst);
      }
    }
  };
};
