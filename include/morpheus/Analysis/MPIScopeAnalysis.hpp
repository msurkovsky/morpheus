
//===----------------------------------------------------------------------===//
//
// Description of the file ... basic idea to provide a scope of elements
// that are involved in MPI either directly or indirectly
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/PassManager.h"

#include <vector>
#include <string>
#include <iterator>

namespace llvm {

  class MPIScopeAnalysis;

  class MPIScopeResult { // TODO: make the data private
    Function *scope; // NOTE: it has to be a function ("worst case" is main)
    CallInst *start; // call that invokes MPI_Init within 'scope'
    CallInst *end;   // call that invokes MPI_Finalize within 'scope'

    CallInst *init_call;     // call of MPI_Init
    CallInst *finalize_call; // call of MPI_Finalize

  public:
    MPIScopeResult() : scope(nullptr), start(nullptr), end(nullptr),
                       init_call(nullptr), finalize_call(nullptr) { };

    MPIScopeResult(Function *scope, CallInst *start, CallInst *end,
                   CallInst* init_call, CallInst *finalize_call)
      : scope(scope), start(start), end(end),
        init_call(init_call), finalize_call(finalize_call) { }

    bool isValid() {
      // Instruction *test_i = init_call->next();
      // auto node = init_call->getNextNode();
      errs() << "\n\tInit call instr: " << *init_call << "\n";

      errs() << "\tisa<Inst>: " << isa<Instruction>(init_call) << "\n";
      // ---------------->>>
      auto next = init_call->getNextNode(); // TODO: It seems that getNextNode is the right instruction I'm looking for!!
      errs() << "\tNext: " << *next << "\n";
      // ----------------<<<

      auto *bb = init_call->getParent();
      // const auto &list = bb->*(BasicBlock::getSublistAccess((Instruction *) nullptr));
      // const auto &list = bb->*(BasicBlock::getSublistAccess(init_call));


      // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
      auto it2 =  init_call->getIterator(); // this is the solution I want to use!
      // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
      // NOTE: the `end` is either nullptr or finalize_call


      // auto next2 = list.getNextNode(init_call);
      // errs() << "\tNxtC: " << *next2 << "\n";
      auto it = bb->begin();
      errs() << "\tIterator: " << *it << "\n";
      errs() << "\tIt2:     " << *it2 << "\n";
      auto next2 = std::next(it2);
      errs() << "\tIt next2: " << *next2 << "\n";
      // auto &it = init_call->iterator;
      return scope != nullptr;
    }
    // NOTE:
    // Well, in the end I need a kind of navigation trough the scope. Therefore, a set of queries needs to be defined.

    // TODO: define it as an iterator over "unfolded" scope
  };


  class MPIScopeAnalysis : public AnalysisInfoMixin<MPIScopeAnalysis> { // both -*-module-*- and function analysis (->4.3.19: I don't think so. *module* is enough)
    static AnalysisKey Key;
    friend AnalysisInfoMixin<MPIScopeAnalysis>;

  public:

    using Result = MPIScopeResult;

    MPIScopeResult run (Module &, ModuleAnalysisManager &);
  };
}
